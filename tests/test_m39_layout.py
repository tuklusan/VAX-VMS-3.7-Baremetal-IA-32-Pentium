"""M39/M40: actual stage-2 size/LBN patching and 1-MiB PC geometry."""
from pathlib import Path
import struct

r=Path(__file__).resolve().parents[1]
s=(r/'src/boot.s').read_text()
boot=(r/'build/boot.bin').read_bytes()
image=(r/'vms37lab.ima').read_bytes()
kernel=(r/'build/kernel.bin').read_bytes()
assert s.count('    call bios_read_retry')==1
assert '    movw stage2_count, %bp' in s and '    movw stage2_lbn, %di' in s
assert '    movw $0x1000, %si' in s and '    addw $32, %si' in s
assert '    movw $36, %bx' in s and '    movb $18, %bl' in s
assert '    movb $1, %al' in s and '    cmpw $2880, %ax' in s
assert '    ljmp $0x08, $0x00010000' in s
assert len(boot)==512 and boot[-2:]==b'\x55\xaa'
assert len(kernel)<=512*512
assert len(image)==1474560
position=boot.find(b'VMSB')
assert 0 <= position <= 500
count,lbn=struct.unpack_from('<HH', image, position+4)
assert 1 <= count <= 512 and 1 <= lbn and lbn+count <= 2880
assert count*512 >= len(kernel)
print('M39/M40 counted stage-2 disk allocation, 1.44MB CHS bounds and 0x10000 entry PASS')
