from pathlib import Path
import subprocess
r=Path(__file__).resolve().parent.parent
k=(r/'src/kernel.c').read_text()
for t in ('NSP OPEN','NSP CLOSE','NSP POLL','NSP RESET','nsp_process_short','port_nsp_link_tick'):
    assert t in k,t
nm=subprocess.check_output(['nm',str(r/'build/kernel.elf')],text=True)
for t in ('port_nsp_link_open','port_nsp_link_rx','port_nsp_link_tick','port_nsp_link_disconnect'):
    assert t in nm,t
print('M17 NSP probe state machine linked/integrated PASS')
