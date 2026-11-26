#!/usr/bin/env python3
"""Execute the *assembled* stage-1 loader with a minimal 8086 BIOS model.

Scope: real-mode instructions until protected-mode CR0 switch; not a guest
kernel emulator. Reject unsupported opcodes instead of silently skipping them.
Checks disk bytes actually copied, CHS read retries, DMA boundaries, A20 gate,
and the persistent-error path against the assembled 512-byte boot sector.
"""
from __future__ import annotations
import argparse
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


class Boot16:
    def __init__(self, image: bytes, failures: dict[int, int] | None = None, drive: int = 0):
        if len(image) != 1474560 or image[510:512] != b'\x55\xaa':
            raise ValueError('not a 1.44MB bootable floppy')
        self.mem = bytearray(4 * 1024 * 1024)
        self.mem[0x7c00:0x7e00] = image[:512]
        self.image = image
        self.reg = dict(ax=0, bx=0, cx=0, dx=drive, si=0, di=0, bp=0, sp=0, ds=0, es=0, ss=0)
        self.boot_drive = drive
        self.reg32 = dict(ax=0,bx=0,cx=0,dx=drive)
        self.e820_ranges = ((0,0x9fc00,1),(0x9fc00,0x60400,2),
                            (0x100000,0x7f00000,1))
        self.e820_calls = 0
        self.ip = 0x7c00
        self.steps = 0
        self.cf = False
        self.zf = False
        self.failed = dict(failures or {})
        self.reads: list[tuple[int,int,int]] = []
        self.reset_count = 0
        self.error_text = bytearray()
        self.a20_port = 0
        self.a20_enabled = False
        self.gdt_seen = False
        self.reached_switch = False
        self.halted = False

    def byte(self):
        a = self.mem[self.ip]
        self.ip += 1
        return a

    def word(self):
        return self.byte() | (self.byte() << 8)

    def r8(self, name):
        r, upper = {'al': ('ax',0), 'ah': ('ax',8), 'ch': ('cx',8),
                    'cl': ('cx',0), 'bl': ('bx',0), 'dh': ('dx',8), 'dl': ('dx',0)}[name]
        return (self.reg[r] >> upper) & 255

    def set8(self, name, val):
        r, upper = {'al': ('ax',0), 'ah': ('ax',8), 'ch': ('cx',8),
                    'cl': ('cx',0), 'bl': ('bx',0), 'dh': ('dx',8), 'dl': ('dx',0)}[name]
        self.reg[r] = (self.reg[r] & ~(255 << upper)) | ((val & 255) << upper)

    def set32(self, name, value):
        self.reg32[name]=value & 0xffffffff
        self.reg[name]=value & 0xffff

    def push(self, val):
        self.reg['sp'] = (self.reg['sp'] - 2) & 65535
        at = self.reg['ss'] * 16 + self.reg['sp']
        self.mem[at:at+2] = (val & 65535).to_bytes(2,'little')

    def pop(self):
        at = self.reg['ss'] * 16 + self.reg['sp']
        val = int.from_bytes(self.mem[at:at+2], 'little')
        self.reg['sp'] = (self.reg['sp'] + 2) & 65535
        return val

    def bios(self, vector):
        ah = self.r8('ah')
        if vector == 0x10:
            if ah == 0x0e:
                self.error_text.append(self.r8('al'))
            elif self.reg['ax'] != 3:
                raise AssertionError(f'unsupported video BIOS AX={self.reg["ax"]:x}')
            return
        if vector == 0x15:
            if self.reg32['ax'] != 0xe820 or self.reg32['dx'] != 0x534d4150:
                raise AssertionError('unsupported INT 15h memory call')
            index=self.reg32['bx']
            if index>=len(self.e820_ranges) or self.reg32['cx']<20:
                self.cf=True
                return
            base, length, typ=self.e820_ranges[index]
            address=self.reg['es']*16+self.reg['di']
            self.mem[address:address+20]=(base.to_bytes(8,'little')+
                                           length.to_bytes(8,'little')+
                                           typ.to_bytes(4,'little'))
            self.set32('ax',0x534d4150)
            self.set32('cx',20)
            self.set32('bx', index+1 if index+1<len(self.e820_ranges) else 0)
            self.e820_calls+=1
            self.cf=False
            return
        if vector != 0x13:
            raise AssertionError(f'unsupported BIOS vector {vector:02x}')
        if self.r8('dl') != self.boot_drive:
            raise AssertionError('boot drive not preserved')
        if ah == 0:
            self.reset_count += 1
            self.cf = False
            self.set8('ah', 0)
            return
        if ah != 2:
            raise AssertionError(f'unsupported BIOS disk function {ah}')
        count = self.r8('al')
        head, cylinder, sector = self.r8('dh'), self.r8('ch'), self.r8('cl')
        if not (1 <= count <= 18 and 1 <= sector <= 18 and
                sector + count - 1 <= 18 and head in (0,1)):
            raise AssertionError('invalid CHS request')
        lba = (cylinder * 2 + head)*18 + sector - 1
        dest = self.reg['es'] * 16 + self.reg['bx']
        size = count * 512
        if dest//65536 != (dest+size-1)//65536:
            raise AssertionError(f'8237 DMA would cross 64-KiB at {dest:x}')
        if self.reg['bx']+size > 65536:
            raise AssertionError('BIOS ES:BX wraps')
        if self.failed.get(lba, 0):
            self.failed[lba] -= 1
            self.cf = True
            self.reg['ax'] = 0x1000  # AH error, AL modified by BIOS
            return
        self.cf = False
        self.mem[dest:dest+size] = self.image[lba*512:(lba+count)*512]
        self.reads.append((lba, count, dest))
        self.reg['ax'] = 0x0000  # AL success is not an authoritative count

    def finish_switch(self):
        """Step the assembled CR0/GDT handoff through its far jump to stage2.

        This narrowly models the actual instruction sequence and selectors;
        it deliberately stops before executing the kernel's 32-bit code.
        """
        assert self.gdt_seen and self.a20_enabled
        # CR0 is a 32-bit register even when accessed in 16-bit code.
        assert bytes(self.mem[self.ip:self.ip+3])==b'\x0f\x20\xc0', 'MOV EAX,CR0'
        self.ip+=3
        assert bytes(self.mem[self.ip:self.ip+3])==b'\x66\x83\xc8', 'OR EAX,imm8'
        self.ip+=3
        value=self.byte()
        assert value==1,('CR0 protected-mode enable bit absent',value)
        cr0=0x60000010|value  # representative reset value, not a CPU emulator
        assert bytes(self.mem[self.ip:self.ip+3])==b'\x0f\x22\xc0', 'MOV CR0,EAX'
        self.ip+=3
        assert cr0&1
        assert self.byte()==0x66 and self.byte()==0xea,'missing 32-bit far jump'
        target=int.from_bytes(self.mem[self.ip:self.ip+4],'little');self.ip+=4
        cs=self.word()
        assert cs==8 and 0x7c00<=target<0x7e00,('wrong CPL0 CS/handoff',cs,target)
        self.ip=target
        assert bytes(self.mem[self.ip:self.ip+2])==b'\x66\xb8','missing MOV AX,selector'
        self.ip+=2
        ax=self.word()
        assert ax==0x10
        regs={0xd8:'ds',0xc0:'es',0xe0:'fs',0xe8:'gs',0xd0:'ss'}
        for mod,reg in regs.items():
            assert self.byte()==0x8e and self.byte()==mod,('wrong protected segment',reg)
            self.reg[reg]=ax
        assert self.byte()==0xbc,'missing MOV ESP,stack top'
        esp=int.from_bytes(self.mem[self.ip:self.ip+4],'little');self.ip+=4
        assert esp==0x9fc00,('unexpected protected kernel stack',hex(esp))
        assert self.byte()==0xea,'missing final far jump'
        entry=int.from_bytes(self.mem[self.ip:self.ip+4],'little');self.ip+=4
        final_cs=self.word()
        assert (entry,final_cs)==(0x10000,8)
        assert self.mem[entry:entry+2]==b'\xfc\x31','incorrect stage2 entry code'
        assert self.mem[entry:entry+2]==self.image[512:514]
        self.ip=entry
        self.reached_switch=True
        self.cr0=cr0
        self.cs=final_cs
        self.esp=esp
        self.protected_entry=True
        return self

    def run(self):
        for self.steps in range(25000):
            if bytes(self.mem[self.ip:self.ip+3])==b'\x0f\x20\xc0':
                return self.finish_switch()
            address = self.ip
            op = self.byte()
            if op == 0x66:
                nextop=self.byte()
                if nextop in (0xb8,0xb9,0xba):
                    value=int.from_bytes(self.mem[self.ip:self.ip+4],'little')
                    self.ip+=4
                    self.set32({0xb8:'ax',0xb9:'cx',0xba:'dx'}[nextop],value)
                elif nextop == 0x31:
                    assert self.byte()==0xdb
                    self.set32('bx',0)
                    self.cf=False
                    self.zf=True
                elif nextop == 0x3d:
                    imm=int.from_bytes(self.mem[self.ip:self.ip+4],'little')
                    self.ip+=4
                    difference=self.reg32['ax']-imm
                    self.cf=difference<0
                    self.zf=difference==0
                elif nextop == 0x83:
                    assert self.byte()==0xf9
                    value=self.byte()
                    difference=self.reg32['cx']-value
                    self.cf=difference<0
                    self.zf=difference==0
                elif nextop == 0x85:
                    assert self.byte()==0xdb
                    self.zf=self.reg32['bx']==0
                    self.cf=False
                else:
                    raise AssertionError(f'unsupported 32-bit prefix 0x{nextop:02x}')
                continue
            if op in (0xfa, 0xfb):  # CLI/STI
                continue
            if op == 0x31:
                mod = self.byte()
                if mod not in (0xc0,0xdb,0xd2):
                    raise AssertionError('unsupported XOR')
                self.reg[{0xc0:'ax',0xdb:'bx',0xd2:'dx'}[mod]] = 0
                self.zf = True
                self.cf = False
            elif op == 0x8e:
                mod = self.byte()
                if mod not in (0xd8, 0xd0, 0xc0):
                    raise AssertionError(f'unsupported segment move {mod:x}')
                self.reg[{0xd8:'ds',0xd0:'ss',0xc0:'es'}[mod]] = self.reg['ax']
            elif op == 0xc7:
                assert self.byte()==0x06
                at=self.word()
                val=self.word()
                self.mem[at:at+2]=val.to_bytes(2,'little')
            elif op == 0xff:
                assert self.byte()==0x06
                at=self.word()
                val=(int.from_bytes(self.mem[at:at+2],'little')+1)&0xffff
                self.mem[at:at+2]=val.to_bytes(2,'little')
                self.zf=val==0
            elif op in (0xb8,0xbb,0xbc,0xbe,0xbf):
                self.reg[{0xb8:'ax',0xbb:'bx',0xbc:'sp',0xbe:'si',0xbf:'di'}[op]] = self.word()
            elif op in (0xb0,0xb1,0xb3,0xb4,0xb5,0xb6):
                self.set8({0xb0:'al',0xb1:'cl',0xb3:'bl',0xb4:'ah',0xb5:'ch',0xb6:'dh'}[op],self.byte())
            elif op in (0x50,0x53,0x51,0x52,0x56,0x06):
                reg={0x50:'ax',0x53:'bx',0x51:'cx',0x52:'dx',0x56:'si',0x06:'es'}[op]
                self.push(self.reg[reg])
            elif op in (0x58,0x5b,0x59,0x5a,0x5e,0x07):
                reg={0x58:'ax',0x5b:'bx',0x59:'cx',0x5a:'dx',0x5e:'si',0x07:'es'}[op]
                self.reg[reg] = self.pop()
            elif op in (0x88,0x8a):
                mod=self.byte()
                if mod==0x16:
                    at=self.reg['ds']*16+self.word()
                    if op==0x88:self.mem[at]=self.r8('dl')
                    else:self.set8('dl',self.mem[at])
                elif op==0x88 and mod in (0xc5,0xc6,0xe1):
                    src,dst={0xc5:('al','ch'),0xc6:('al','dh'),0xe1:('ah','cl')}[mod]
                    self.set8(dst,self.r8(src))
                else:raise AssertionError('unexpected byte move')
            elif op == 0x8b:
                mod=self.byte()
                if mod not in (0x2e,0x3e):
                    raise AssertionError('unexpected MOV from memory')
                at=self.word()
                self.reg[{0x2e:'bp',0x3e:'di'}[mod]] = int.from_bytes(self.mem[at:at+2], 'little')
            elif op == 0x85:
                mod=self.byte()
                if mod==0xed:self.zf=self.reg['bp']==0
                elif mod==0xdb:self.zf=self.reg['bx']==0
                else:raise AssertionError('unsupported TEST')
                self.cf=False
            elif op == 0x89:
                mod=self.byte()
                src={0xf8:'di',0xd0:'dx',0xf0:'si'}.get(mod)
                if src is None:raise AssertionError('unexpected MOV between registers')
                self.reg['ax']=self.reg[src]
            elif op == 0x81:
                mod=self.byte()
                if mod==0xfd:lhs=self.reg['bp']
                elif mod==0xff:lhs=self.reg['di']
                else:raise AssertionError('unsupported CMP')
                value=lhs-self.word()
                self.zf=(value&0xffff)==0
                self.cf=value<0
            elif op == 0x83:
                mod=self.byte()
                imm=self.byte()
                if mod==0xff:
                    value=self.reg['di']-imm
                    self.zf=(value&0xffff)==0
                    self.cf=value<0
                elif mod==0xc6:
                    self.reg['si']=(self.reg['si']+imm)&0xffff
                elif mod==0xc7:
                    self.reg['di']=(self.reg['di']+imm)&0xffff
                    self.zf=self.reg['di']==0
                else:raise AssertionError('unexpected 83 instruction')
            elif op == 0x3d:
                value=self.reg['ax']-self.word()
                self.zf=(value&0xffff)==0
                self.cf=value<0
            elif op == 0x01:
                assert self.byte()==0xe8
                value=self.reg['ax']+self.reg['bp']
                self.reg['ax']=value&0xffff
                self.cf=value>0xffff
                self.zf=self.reg['ax']==0
            elif op == 0xf7:
                assert self.byte()==0xf3
                divisor=self.reg['bx']
                assert divisor>0
                value=(self.reg['dx']<<16)|self.reg['ax']
                assert value//divisor<=0xffff
                self.reg['ax'],self.reg['dx']=divmod(value,divisor)
            elif op == 0xf6:
                assert self.byte()==0xf3
                divisor=self.r8('bl')
                assert divisor>0
                quo,rem=divmod(self.reg['ax'],divisor)
                assert quo<=255
                self.set8('al',quo)
                self.set8('ah',rem)
            elif op == 0xfe:
                assert self.byte()==0xc1
                self.set8('cl',self.r8('cl')+1)
            elif op == 0x47:
                self.reg['di']=(self.reg['di']+1)&0xffff
                self.zf=self.reg['di']==0
            elif op == 0x4d:
                self.reg['bp']=(self.reg['bp']-1)&0xffff
                self.zf=self.reg['bp']==0
            elif op == 0xe8:
                disp=self.word()
                self.push(self.ip - 0x7c00)
                self.ip=(self.ip + (disp if disp<32768 else disp-65536)) & 65535
            elif op == 0xc3:
                self.ip=self.pop()+0x7c00
            elif op in (0x72,0x73,0x75,0x74,0xeb):
                disp=self.byte()
                signed=disp if disp<128 else disp-256
                take=(op==0xeb or op==0x72 and self.cf or op==0x73 and not self.cf or
                      op==0x75 and not self.zf or op==0x74 and self.zf)
                if take:self.ip += signed
            elif op == 0x4e:
                self.reg['si']=(self.reg['si']-1)&65535
                self.zf=self.reg['si']==0
            elif op == 0xcd:
                self.bios(self.byte())
            elif op == 0xe4:
                assert self.byte()==0x92
                self.set8('al', self.a20_port)
            elif op == 0xe6:
                assert self.byte()==0x92
                self.a20_port=self.r8('al')
                self.a20_enabled=bool(self.a20_port & 2) and not bool(self.a20_port & 1)
            elif op == 0x0c:
                self.set8('al',self.r8('al')|self.byte())
            elif op == 0x24:
                self.set8('al',self.r8('al')&self.byte())
            elif op == 0x0f:
                if self.mem[self.ip] in (0x82,0x84,0x87):
                    condition=self.byte()
                    offset=self.word()
                    if offset>=32768:offset-=65536
                    taken=(condition==0x82 and self.cf or
                           condition==0x84 and self.zf or
                           condition==0x87 and not self.cf and not self.zf)
                    if taken:self.ip+=offset
                    continue
                if self.mem[self.ip:self.ip+2]!=b'\x01\x16':
                    raise AssertionError('unsupported protected-mode opcode')
                self.ip+=2
                at=self.word()
                limit=int.from_bytes(self.mem[at:at+2], 'little')
                base=int.from_bytes(self.mem[at+2:at+6], 'little')
                if limit!=23 or base < 0x7c00 or base + limit >= 0x7e00:
                    raise AssertionError('GDT descriptor invalid')
                self.gdt_seen=True
            elif op==0xac:
                at=self.reg['ds']*16 + self.reg['si']
                self.set8('al',self.mem[at])
                self.reg['si']+=1
            elif op==0x84:
                assert self.byte()==0xc0
                self.zf=self.r8('al')==0
            elif op==0xf4:
                self.halted=True
                return self
            else:
                raise AssertionError(f'unsupported opcode {op:02x} at {address:x}')
        raise AssertionError('stage-1 exceeded bounded instruction count')


def verify(image: bytes):
    sector=image[:512]
    at=sector.find(b'VMSB')
    assert 0<at<500 and at==sector.rfind(b'VMSB')
    count=int.from_bytes(sector[at+4:at+6], 'little')
    first=int.from_bytes(sector[at+6:at+8], 'little')
    assert first>=1 and 1<=count<=512 and first+count<=2880
    expected=image[first*512:(first+count)*512]
    expected_lbas=[(n,1,0x10000+(n-first)*512) for n in range(first,first+count)]
    bad_lba=first+count//2
    cases=[({},'normal floppy',0,False,0),({},'other BIOS boot drive',0,False,0x80),
           ({first:1},'first-sector retry',1,False,0),
           ({bad_lba:1},'mid-file retry',1,False,0),
           ({bad_lba:3},'persistent media error',3,True,0)]
    for failures,name,resets,error_expected,drive in cases:
        sim=Boot16(image,failures,drive).run()
        assert sim.reset_count==resets,(name,sim.reset_count,resets)
        if error_expected:
            assert sim.halted and not sim.reached_switch,(name,'missing error halt')
            assert sim.error_text==b'Disk read failed.\r\n',(name,sim.error_text)
            assert sim.reads==expected_lbas[:count//2],name
        else:
            assert sim.reached_switch and sim.gdt_seen and sim.a20_enabled,name
            assert sim.protected_entry and sim.ip==0x10000 and sim.esp==0x9fc00,name
            assert sim.cr0&1 and sim.cs==8,name
            assert sim.reg['sp']==0x7c00,('boot stack leak',name)
            assert sim.e820_calls==3 and int.from_bytes(sim.mem[0x4ff0:0x4ff2],'little')==3,('BIOS E820',name)
            assert int.from_bytes(sim.mem[0x5000+40:0x5000+44],'little')==0x100000,('E820 base',name)
            assert sim.mem[0x10000:0x10000+count*512]==expected,('stage2 bytes',name)
            assert sim.reads==expected_lbas,('CHS map',name,sim.reads)
    print('BOOTBLOCK assembled BIOS loader, disk count, retries and protected handoff PASS')


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--image',type=Path,default=ROOT/'vms37lab.ima')
    args=parser.parse_args()
    verify(args.image.read_bytes())

if __name__=='__main__':main()
