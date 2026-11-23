"""M12 device driver wiring/layout; not a guest boot test."""
from pathlib import Path
import re, subprocess
r=Path(__file__).resolve().parent.parent
src=(r/'src/kernel.c').read_text()
for name in ('TEST NETIO','POLL NIC','port_ne2k_x86_recv','port_ne2k_x86_send'):
    assert name in src
src2=(r/'src/vms_ne2k.c').read_text()
for name in ('CR_RWRITE=0x12','REG_TBCR0=5','REG_TBCR1=6','ISR_RDC=0x40',
             'port_ne2k_send','port_ne2k_recv','nextpage','port_ne2k_pending'):
    assert name in src2
out=subprocess.check_output(['nm','-n',str(r/'build/kernel.elf')],text=True)
for sym in ('port_ne2k_send','port_ne2k_recv','port_ne2k_x86_send',
            'port_ne2k_x86_recv','__bss_end'):
    assert re.search(r'\b'+sym+r'\b',out)
print('M12 NE2000 static PASS: remote-DMA, frame queues, monitor wiring')
