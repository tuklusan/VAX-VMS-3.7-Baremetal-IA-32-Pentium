from pathlib import Path
import subprocess
r=Path(__file__).resolve().parent.parent
c=(r/'src/arch.c').read_text()
assert 'port_user_copy_to(&m,f->ebx' in c
assert 'port_user_copyv_to(&m,both,2u' in c
assert 'port_user_copy_to(&m,arch_io_iosb_dst' in c
assert '*(uint32_t *)(uintptr_t)f->ebx=' not in c
assert '*(struct port_qio_iosb *)(uintptr_t)arch_io_iosb_dst=' not in c
nm=subprocess.check_output(['nm',str(r/'build/kernel.elf')],text=True)
assert 'port_user_copy_to' in nm
assert len((r/'build/kernel.bin').read_bytes())<=260*512
print('M30 translated CPL3 result-copy linked PASS')
