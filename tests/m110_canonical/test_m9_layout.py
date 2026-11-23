"""M9 event flag integration checks; guest boot still unverified."""
from pathlib import Path
import re,subprocess
r=Path(__file__).resolve().parent.parent
src=(r/'src/arch.c').read_text();asm=(r/'src/arch.s').read_text()
k=(r/'src/kernel.c').read_text()
for x in ['arch_run_event_test','PORT_PCB_WAIT_EF','port_postef_wait',
          'port_postef_post_mask','port_event_satisfied','event_wakes']:
    assert x in src
# M51 implementation moved wake/block decisions into the shared POSTEF engine.
backend=(r/'src/vms_postef.c').read_text()
for x in ['port_pcb_block_ef','port_pcb_wake_ef','port_event_satisfied']:
    assert x in backend
for x in ['arch_user_event_a','arch_user_event_b','WAIT_AND','WAIT_OR']:
    assert x in asm
for x in ['TEST EVENT','SHOW EVENT']:
    assert x in k
symbols={}
for l in subprocess.check_output(['nm','-n',str(r/'build/kernel.elf')],text=True).splitlines():
    m=re.match(r'^([0-9A-Fa-f]+)\s+\S\s+(\S+)',l)
    if m:symbols[m[2]]=int(m[1],16)
for n in ['arch_user_event_a','arch_user_event_b','port_event_set',
          'arch_run_event_test','__bss_end']:
    assert n in symbols
assert symbols['__bss_end']<0x90000
print('M9 static checks PASS: kernel wait/ready, shared EF, CPL3 user probes')
