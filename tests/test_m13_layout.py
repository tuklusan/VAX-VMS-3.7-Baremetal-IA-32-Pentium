"""M13 binary wiring and raw Ethernet identifiers."""
from pathlib import Path
import subprocess
r=Path(__file__).resolve().parent.parent
k=(r/'src/kernel.c').read_text()
for text in ('TEST DECNET','DECNET TX','POLL DECNET','port_ne2k_x86_set_station'):
    assert text in k
n=(r/'src/vms_decnet.c').read_text()
for text in ('0x60','0x03','0xaa','6+len','wire_len+16u>len'):
    assert text in n
symbols=subprocess.check_output(['nm',str(r/'build/kernel.elf')],text=True)
for symbol in ('port_decnet_build_short','port_decnet_parse_short','port_ne2k_set_station'):
    assert symbol in symbols
print('M13 DECnet short routing/driver integration static PASS')
