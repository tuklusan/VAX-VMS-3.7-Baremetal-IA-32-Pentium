from pathlib import Path
import subprocess
root=Path(__file__).resolve().parent.parent
arch=(root/'src/arch.c').read_text()
a=(root/'src/arch.s').read_text()
k=(root/'src/kernel.c').read_text()
for x in ['port_user_range_valid','port_channel_assign','port_channel_deassign',
          'f->eax==18u','f->eax==19u','user_channel_result']:
 assert x in arch,x
assert '.global arch_user_channel_probe' in a
for x in ['TEST USRCHAN','SHOW USRCHAN']:
 assert x in k,x
nm=subprocess.check_output(['nm',str(root/'build/kernel.elf')],text=True)
for x in ['arch_run_user_channel_test','arch_user_channel_probe',
          'arch_user_channel_result','port_channel_deassign']:
 assert x in nm,x
assert len((root/'build/kernel.bin').read_bytes()) <= 260*512
print('M28 native CPL3 channel assign/deassign ABI linked PASS')
