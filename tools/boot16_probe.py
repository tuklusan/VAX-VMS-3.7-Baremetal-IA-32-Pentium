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
        assert esp==0x9f000,('unexpected protected kernel stack',hex(esp))
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
                    