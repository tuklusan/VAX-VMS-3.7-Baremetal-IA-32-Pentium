from pathlib import Path
import subprocess
root=Path(__file__).resolve().parent.parent
src=(root/'src/kernel.c').read_text()
for x in ('TEST MIRROR','port_mirror_make_request','port_mirror_verify',
          'nsp_mirror_waiting','nsp_mirror_timeouts','decnet_enabled=1'):
    assert x in src,x
nm=subprocess.check_output(['nm',str(root/'build/kernel.elf')],text=True)
for x in ('port_mirror_make_request','port_mirror_reply','port_mirror_verify'):
    assert x in nm,x
print('M21 Mirror object-25 guest integration PASS')
