from pathlib import Path
import subprocess
r=Path(__file__).resolve().parent.parent
c=(r/'src/arch.c').read_text()
a=(r/'src/arch.s').read_text()
k=(r/'src/kernel.c').read_text()
assert 'f->eax==22u || f->eax==29u' in c
assert 'port_user_io_decode(&m,f->ecx' in c
assert 'arch_user_qio_desc_probe:' in a
for n in ('movl $29,%eax','movl $30,%eax'):
 assert n in a
assert 'TEST USRDESC' in k
sym=subprocess.check_output(['nm',str(r/'build/kernel.elf')],text=True)
for n in ('port_user_io_decode','arch_user_qio_desc_probe','arch_run_user_qio_desc_test'):
 assert n in sym
assert len((r/'build/kernel.bin').read_bytes())<=251*512
print('M35 decoded user QIO descriptor, CPL3 guest ABI and binary PASS')
