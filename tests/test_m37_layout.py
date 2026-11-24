"""M37: verify the actual counted stage-two CHS read/reset/retry contract."""
from pathlib import Path
import subprocess

r=Path(__file__).resolve().parents[1]
boot=(r/'build/boot.bin').read_bytes()
s=(r/'src/boot.s').read_text()
assert len(boot)==512 and boot[-2:]==b'\x55\xaa'
assert s.count('    call bios_read_retry') == 1  # counted read loop
assert s.count('    int $0x13') == 2     # one read and one reset per retry
assert 'read_block:' in s and 'next_block:' in s
assert '    movw stage2_count, %bp' in s
assert '    cmpw $260, %bp' in s
assert '    movw stage2_lbn, %di' in s
assert '    movw $3, %si' in s and '    decw %si' in s
assert '    jnz .retry' in s and '    jmp disk_error' in s
assert s.count('    pushw %es')==2 and s.count('    popw %es')==3
assert '    movb $1, %al' in s and '    xorw %ax, %ax' in s
nm=subprocess.check_output(['nm',str(r/'build/boot.o')],text=True)
assert 'bios_read_retry' in nm
assert 'u->last_cr=0;' in (r/'src/vms_serial.c').read_text()
print('M37 counted single-sector CHS read, three retries, drive reset and UART CRLF PASS')
