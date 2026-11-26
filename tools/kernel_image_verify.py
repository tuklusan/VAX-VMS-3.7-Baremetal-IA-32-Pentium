#!/usr/bin/env python3
"""Independent ELF32 PT_LOAD, raw stage2, image and BSS layout consistency.
Static validation only; it does not execute the protected-mode kernel.
"""
from pathlib import Path
import struct
import subprocess
ROOT=Path(__file__).resolve().parents[1]
BOOT_LOAD_LIMIT=0x10000+512*512

def load_segments(blob):
    assert blob[:7]==b'\x7fELF\x01\x01\x01', 'wrong ELF byte order/class'
    etype, machine=struct.unpack_from('<HH',blob,16)
    entry,phoff=struct.unpack_from('<II',blob,24)
    phentsize,phnum=struct.unpack_from('<HH',blob,42)
    assert (etype,machine,entry,phentsize)==(2,3,0x10000,32)
    assert 1<=phnum<=8
    seg=[]
    for i in range(phnum):
        typ,off,virtual,physical,filesz,memsz,flags,align=struct.unpack_from('<8I',blob,phoff+i*32)
        if typ!=1:continue
        assert filesz<=memsz and off+filesz<=len(blob)
        assert virtual==physical and 0x10000<=physical
        assert physical+memsz<=0x9e000,('BSS reaches boot stack',physical+memsz)
        assert physical+filesz<=BOOT_LOAD_LIMIT,('not loaded by BIOS',physical+filesz)
        assert align in (1,4096) and off%align==virtual%align
        assert flags in (5,6),('unexpected executable/writable mapping',flags)
        seg.append((physical,filesz,memsz,blob[off:off+filesz],flags))
    assert len(seg)==2 and [s[4] for s in seg]==[5,6]
    for i,x in enumerate(seg):
        for y in seg[i+1:]:
            assert x[0]+x[2]<=y[0] or y[0]+y[2]<=x[0]
    return seg

def verify():
    elf=(ROOT/'build/kernel.elf').read_bytes()
    flat=(ROOT/'build/kernel.bin').read_bytes()
    boot=(ROOT/'build/boot.bin').read_bytes()
    image=(ROOT/'vms37lab.ima').read_bytes()
    assert len(boot)==512 and boot[-2:]==b'\x55\xaa'
    assert len(image)==1474560
    marker=boot.find(b'VMSB')
    assert 0<marker<500 and image[:marker+4]==boot[:marker+4]
    assert image[marker+6:512]==boot[marker+6:512]
    assert int.from_bytes(image[marker+4:marker+6],'little')==(len(flat)+511)//512
    assert 0<len(flat)<=512*512
    assert image[512:512+len(flat)]==flat
    assert not any(image[512+len(flat):]),'unexpected data beyond linked image'
    segments=load_segments(elf)
    for phys,filesz,memsz,data,flags in segments:
        start=phys-0x10000
        assert flat[start:start+filesz]==data,('ELF/flat mismatch',hex(phys))
        assert image[512+start:512+start+filesz]==data,('ELF/floppy mismatch',hex(phys))
    symbols=subprocess.check_output(['nm','-n',str(ROOT/'build/kernel.elf')],text=True)
    syms={a[2]:int(a[0],16) for line in symbols.splitlines()
          if len(a:=line.split())==3 and a[1]!='U'}
    assert syms['__bss_start']<syms['__bss_end']<=0x9e000
    assert all(x[0]+x[1]<=syms['__bss_start'] for x in segments)
    assert syms['arch_user_code_begin']%4096==0
    assert segments[0][0]<=syms['arch_user_code_begin']<segments[0][0]+segments[0][1]
    assert len(flat)==max(x[0]-0x10000+x[1] for x in segments)
    print('M43 ELF32 PT_LOAD vs boot image, BSS, W^X and CPL3 alignment PASS')

if __name__=='__main__':verify()
