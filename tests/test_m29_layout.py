from pathlib import Path
import subprocess
root=Path(__file__).resolve().parent.parent
arch=(root/'src/arch.c').read_text(); a=(root/'src/arch.s').read_text()
k=(root/'src/kernel.c').read_text()
for t in ('f->eax==22u','f->eax==23u','port_channel_qio_submit',
          'port_qio_run_one','port_channel_qio_release',
          'port_user_range_valid'):
 assert t in arch,t
for t in ('.global arch_user_qio_probe','subl $544,%esp', 'movl $22,%eax',
          'movl $23,%eax'):
 assert t in a,t
for t in ('TEST USRREAD','SHOW USRREAD','arch_run_user_qio_test'):
 assert t in k,t
nm=subprocess.check_output(['nm',str(root/'build/kernel.elf')],text=True)
for t in ('arch_run_user_qio_test','arch_user_qio_probe',
          'port_channel_qio_submit','port_user_range_valid'):
 assert t in nm,t
assert len((root/'build/kernel.bin').read_bytes())<=273*512
print('M29 CPL3 QIO submit/poll with user-buffer checks linked PASS')
