from pathlib import Path
import subprocess
root=Path(__file__).resolve().parent.parent
k=(root/'src/kernel.c').read_text()
for t in ['TEST CHANNEL','SHOW CHANNEL','port_channel_qio_submit',
          'port_channel_qio_release','port_channel_deassign','PORT_CHANNEL_BUSY']:
    assert t in k,t
symbol=subprocess.check_output(['nm',str(root/'build/kernel.elf')],text=True)
for t in ('port_channel_assign','port_channel_qio_submit','port_channel_qio_cancel',
          'port_channel_qio_release','port_channel_deassign'):
    assert t in symbol,t
assert len((root/'build/kernel.bin').read_bytes())<=260*512
print('M26 owned QIO channel guest integration PASS')
