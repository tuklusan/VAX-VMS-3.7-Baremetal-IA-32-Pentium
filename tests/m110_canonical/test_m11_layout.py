"""M11 NE2000 guest wiring/static checks, not a guest execution test."""
from pathlib import Path
import re,subprocess
r=Path(__file__).resolve().parent.parent
src=(r/'src/kernel.c').read_text()
conf=(r/'dosbox-x.conf').read_text()
for n in ['TEST NIC','SHOW NIC','port_ne2k_x86_probe','port_ne2k_x86_pending']:
    assert n in src
assert 'ne2000 = true' in conf
assert 'nicbase = 300' in conf
assert 'nicirq = 10' in conf
s=(r/'src/vms_ne2k.c').read_text()
for n in ['CR_STOP=0x21','REG_DMA=16','REG_RESET=31',
          'REG_BNRY','REG_CURR','REG_IMR','CR_PAGE1']:
    assert n in s
symbols={}
for l in subprocess.check_output(['nm','-n',str(r/'build/kernel.elf')],text=True).splitlines():
    m=re.match(r'^([0-9a-fA-F]+)\s+\S\s+(\S+)$',l)
    if m:symbols[m[2]]=int(m[1],16)
for name in ['port_ne2k_probe','port_ne2k_pending','port_ne2k_x86_probe',
             'port_ne2k_x86_pending','__bss_end']:
    assert name in symbols
assert symbols['__bss_end']<0x9e000
print('M11 NE2000 static checks PASS: I/O driver and guest commands')
