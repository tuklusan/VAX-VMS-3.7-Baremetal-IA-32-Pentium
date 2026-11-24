from pathlib import Path
import subprocess
r=Path(__file__).resolve().parent.parent
arch=(r/'src/arch.c').read_text();asm=(r/'src/arch.s').read_text();kernel=(r/'src/kernel.c').read_text()
for marker in ('port_channels_teardown_owner','arch_user_qio_teardown_probe','arch_run_user_qio_teardown_test'):
 assert marker in arch
for marker in ('.global arch_user_qio_teardown_probe','movl $22,%eax'):
 assert marker in asm
for marker in ('TEST USRTEAR','SHOW USRTEAR'):
 assert marker in kernel
nm=subprocess.check_output(['nm',str(r/'build/kernel.elf')],text=True)
for marker in ('port_channels_teardown_owner','arch_user_qio_teardown_probe','arch_run_user_qio_teardown_test'):
 assert marker in nm
assert len((r/'build/kernel.bin').read_bytes())<=260*512
print('M32 CPL3 process lifetime QIO cleanup linked PASS')
