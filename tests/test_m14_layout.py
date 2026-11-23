"""M14: normal kernel context, no DMA or protocol code in the IRQ stub."""
from pathlib import Path
import subprocess
r=Path(__file__).resolve().parent.parent
src=(r/'src/kernel.c').read_text()
for term in ('TEST HELLO','HELLO TX','DECNET ON','DECNET OFF','SHOW DECNET',
             'POLL HELLO','decnet_service()','PORT_DECNET_HELLO_INTERVAL_TICKS'):
    assert term in src
# Service is invoked from polled getkey_mode(), not PIT IRQ0.
assert 'static char getkey_mode(int preserve_case)' in src
assert 'decnet_service(); continue;' in src
for term in ('port_decnet_build_endnode_hello','port_decnet_parse_endnode_hello'):
    assert term in subprocess.check_output(['nm',str(r/'build/kernel.elf')],text=True)
print('M14 Hello static PASS: polling loop integration, timer and symbols')
