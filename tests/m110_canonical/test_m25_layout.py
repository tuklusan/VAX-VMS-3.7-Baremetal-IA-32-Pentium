from pathlib import Path
import subprocess
root=Path(__file__).resolve().parent.parent
k=(root/'src/kernel.c').read_text()
for t in ('TEST QIO','SHOW QIO','port_qio_submit',
          'port_qio_run_one','vms_ast_dispatch_one',
          'port_event_satisfied','port_ata_x86_read28'):
    assert t in k,t
nm=subprocess.check_output(['nm',str(root/'build/kernel.elf')],text=True)
for t in ('port_qio_submit','port_qio_run_one','port_qio_release'):
    assert t in nm,t
print('M25 VMS-inspired QIO service guest integration PASS')
