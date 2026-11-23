"""M40 DMA test for the current counted, one-sector BIOS INT 13h loop.

The historical 15 fixed requests no longer exist. The live BOO$BLOCK
reads one 512-byte sector per iteration, with ES advancing by 0x20.
"""
from pathlib import Path
import struct

ROOT = Path(__file__).resolve().parents[1]
src = (ROOT/'src/boot.s').read_text()
boot = (ROOT/'build/boot.bin').read_bytes()
image = (ROOT/'vms37lab.ima').read_bytes()
kernel = (ROOT/'build/kernel.bin').read_bytes()
assert len(boot)==512 and boot[-2:]==b'\x55\xaa'
assert len(image)==2880*512
assert src.count('    call bios_read_retry')==1
for instruction in ('    movw $36, %bx', '    divw %bx', '    movb $18, %bl',
                    '    divb %bl', '    movb $1, %al', '    movw $0x1000, %si',
                    '    addw $32, %si', '    incw %di', '    decw %bp',
                    '    jnz read_block', '    cmpw $251, %bp'):
    assert instruction in src,instruction
pos=boot.find(b'VMSB')
assert 0<=pos<=500
assert image[:pos+4]==boot[:pos+4]
count,first=struct.unpack_from('<HH', image, pos+4)
assert 1 <= first < 2880 and 1<=count<=251 and first+count<=2880
assert (count-1)*512 < len(kernel) <= count*512
assert image[pos+8:512] == boot[pos+8:512]
for index in range(count):
    lba=first+index
    track,within=divmod(lba,36)
    head,sector0=divmod(within,18)
    assert 0<=track<80 and head in (0,1) and 1<=sector0+1<=18
    assert (track*2+head)*18+sector0==lba
    phys=0x10000+index*512
    assert phys>=0x10000 and phys+512<=0x10000+251*512
    assert phys//65536==(phys+511)//65536,('DMA boundary',hex(phys))
    assert len(image[lba*512:(lba+1)*512])==512
# Exercise the maximum supported count at each 64KiB DMA boundary too.
for index in range(251):
    addr=0x10000+index*512
    assert addr//65536==(addr+511)//65536
print(f'M40: {count} counted single-sector CHS reads, geometry and 64KiB DMA bounds PASS')
