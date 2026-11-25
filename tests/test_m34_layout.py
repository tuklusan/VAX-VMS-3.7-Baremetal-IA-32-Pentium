from pathlib import Path
import subprocess
r=Path(__file__).resolve().parent.parent
c=(r/'src/arch.c').read_text()
assert 'port_user_copyv_to(&m,both,2u' in c
assert 'port_ranges_overlap(data_va,512u' in c
assert 'port_user_copy_to(&m,arch_io_dst' not in c
assert '(uint32_t)PORT_QIO_INVALID' in c
s=subprocess.check_output(['nm',str(r/'build/kernel.elf')],text=True)
assert 'port_user_copyv_to' in s
assert len((r/'build/kernel.bin').read_bytes())<=273*512
print('M34 data+IOSB transactional syscall output linked PASS')
