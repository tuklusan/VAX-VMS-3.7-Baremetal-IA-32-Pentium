from pathlib import Path
import subprocess
r=Path(__file__).resolve().parent.parent
s=(r/'src/arch.c').read_text()
assert 'port_user_outputs_disjoint(&m,data_va,512u,iosb_va' in s
assert 'port_channel_qio_submit' in s
assert s.index('port_user_outputs_disjoint(&m,data_va') < s.index('port_channel_qio_submit(&arch_service_channels')
sym=subprocess.check_output(['nm',str(r/'build/kernel.elf')],text=True)
assert 'port_user_outputs_disjoint' in sym
assert len((r/'build/kernel.bin').read_bytes())<=251*512
print('M38 physical overlap rejected before ring3 block I/O queue PASS')
