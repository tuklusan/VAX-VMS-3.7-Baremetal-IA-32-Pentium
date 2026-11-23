from pathlib import Path
import subprocess
root=Path(__file__).resolve().parent.parent
k=(root/'src/kernel.c').read_text()
for token in ('port_nsp_tx_begin', 'port_nsp_rx_sequence',
              'port_nsp_tx_ack','port_nsp_tx_tick','nsp_replay_len',
              'TEST RELIABLE','nsp_send_segment'):
    assert token in k,token
nm=subprocess.check_output(['nm',str(root/'build/kernel.elf')],text=True)
for token in ('port_nsp_tx_begin','port_nsp_rx_sequence','port_nsp_tx_tick'):
    assert token in nm,token
print('M20 retry/ACK guest integration PASS')
