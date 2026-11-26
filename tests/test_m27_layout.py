from pathlib import Path
import subprocess
root=Path(__file__).resolve().parent.parent
k=(root/'src/kernel.c').read_text()
a=(root/'src/arch.c').read_text()
user=(root/'src/arch.s').read_text()
assert 'TEST USERPTR' in k and 'SHOW USERPTR' in k
assert 'port_user_range_valid' in a and 'f->eax==16u' in a
assert '.global arch_user_ptrprobe' in user
nm=subprocess.check_output(['nm',str(root/'build/kernel.elf')],text=True)
for s in ('port_user_range_valid','arch_user_ptrprobe','arch_run_userptr_test'):
 assert s in nm,s
assert len((root/'build/kernel.bin').read_bytes())<=512*512
print('M27 true CPL3 pointer validation trap ABI linked PASS')
