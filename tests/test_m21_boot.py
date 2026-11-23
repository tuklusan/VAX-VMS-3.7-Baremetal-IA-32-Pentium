from pathlib import Path
import struct

root = Path(__file__).resolve().parent.parent
source = (root/'src/boot.s').read_text()
boot = (root/'build/boot.bin').read_bytes()
kernel = (root/'build/kernel.bin').read_bytes()
floppy = (root/'vms37lab.ima').read_bytes()
assert len(boot) == 512 and boot[-2:] == b'\x55\xaa'
assert 0 < len(kernel) <= 251*512
assert source.count('call bios_read_retry') == 1, 'One repeated single-sector CHS read loop'
for instruction in ('movw $0x1000, %si', 'movw $251, %bp',
                    'movw stage2_count, %bp', 'movw stage2_lbn, %di',
                    'movb $1, %al', 'int $0x13', 'ljmp $0x08, $0x00010000',
                    'vmb_e820_next:', 'vmb_e820_invalid:', 'bios_read_retry:',
                    'disk_error:', '0xaa55'):
    if instruction == 'movw $251, %bp':
        continue  # Explicit 251-block bound is CMP, not an unbounded MOV.
    assert instruction in source, instruction
assert 'cmpw $251, %bp' in source
assert 'cmpw $2880, %ax' in source
assert 'jnz read_block' in source
assert 'movw $3, %si' in source and 'jnz .retry' in source
# The M21 $8e00/$b200/$d600 hardcoded segment assumptions were removed
# when SYSBOOT began reading an explicitly counted contiguous stage-2.
assert all(old not in source for old in ('$0x8e00','$0xb200','$0xd600'))
position = boot.find(b'VMSB')
assert 0 <= position <= 510-8, 'Recorded sector stage-2 parameters'
# build/boot.bin is an unpatched template; the imaged floppy has live count.
assert floppy[:position+4] == boot[:position+4]
count, lbn = struct.unpack_from('<HH', floppy, position+4)
assert 1 <= count <= 251 and 1 <= lbn < 2880
assert lbn+count <= 2880
assert count*512 >= len(kernel), 'Recorded disk allocation must fit stage-2'
print('Current live counted CHS boot loader and bounds PASS')
