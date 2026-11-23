from pathlib import Path
import subprocess
r=Path(__file__).resolve().parent.parent
k=(r/'src/kernel.c').read_text()
for t in ('TEST DATA','NSP PING','nsp_data_received',
          'port_nsp_data_ack_build','nsp_data_last_sequence'):
    assert t in k,t
nm=subprocess.check_output(['nm',str(r/'build/kernel.elf')],text=True)
for t in ('port_nsp_data_build','port_nsp_data_parse','port_nsp_data_ack_build',
          'port_nsp_data_ack_parse'):
    assert t in nm,t
print('M19 NSP DATA/ACK symbols and guest integration PASS')
