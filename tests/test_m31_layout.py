from pathlib import Path
import subprocess
r=Path(__file__).resolve().parent.parent
c=(r/'src/arch.c').read_text();a=(r/'src/arch.s').read_text();k=(r/'src/kernel.c').read_text()
for marker in ('f->eax==25u','f->eax==26u','port_channel_qio_cancel','port_channel_qio_release','arch_user_qio_cancel_probe'):
 assert marker in c,marker
for marker in ('.global arch_user_qio_cancel_probe','movl $25,%eax','movl $26,%eax'):
 assert marker in a,marker
for marker in ('TEST USRCANCEL','SHOW USRCANCEL','arch_run_user_qio_cancel_test'):
 assert marker in k,marker
nm=subprocess.check_output(['nm',str(r/'build/kernel.elf')],text=True)
for symbol in ('arch_user_qio_cancel_probe','arch_run_user_qio_cancel_test','port_user_copy_to'):
 assert symbol in nm,symbol
assert len((r/'build/kernel.bin').read_bytes()) <= 265*512
print('M31 CPL3 cancel+release guest path linked PASS')
