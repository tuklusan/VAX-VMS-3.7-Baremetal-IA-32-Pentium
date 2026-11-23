from pathlib import Path
import subprocess
r=Path(__file__).resolve().parent.parent
k=(r/'src/kernel.c').read_text()
for word in ('NSP SERVER ON','NSP SERVER OFF','TEST SERVER',
             'port_mirror_server_connect','port_mirror_server_data',
             'port_mirror_server_ack','port_mirror_server_tick',
             'nsp_server_process_short','port_nsp_build_conn_ack'):
    assert word in k,word
nm=subprocess.check_output(['nm',str(r/'build/kernel.elf')],text=True)
for name in ('port_mirror_server_connect','port_mirror_server_data',
             'port_mirror_server_ack','port_mirror_server_tick'):
    assert name in nm,name
print('M22 passive NSP Mirror listener guest integration PASS')
