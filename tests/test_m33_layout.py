from pathlib import Path
import subprocess
r=Path(__file__).resolve().parent.parent
c=(r/'src/arch.c').read_text()
a=(r/'src/arch.s').read_text()
k=(r/'src/kernel.c').read_text()
assert 'port_user_copy_from(&m,&value,f->ebx' in c
assert 'arch_user_copyin_probe:' in a
assert 'TEST USRCOPYIN' in k
sym=subprocess.check_output(['nm',str(r/'build/kernel.elf')],text=True)
for n in ('port_user_copy_from','arch_user_copyin_probe','arch_run_user_copyin_test'):
 assert n in sym,n
assert len((r/'build/kernel.bin').read_bytes())<=251*512
print('M33 CPL3 copy-in wireup and binary layout PASS')
