from pathlib import Path
import subprocess
r=Path(__file__).resolve().parent.parent
k=(r/'src/kernel.c').read_text()
for term in ('TEST NSP','SHOW NSP','port_nsp_parse(shortpkt->payload',
             'nsp_parsed','nsp_rejected'):
    assert term in k,term
syms=subprocess.check_output(['nm',str(r/'build/kernel.elf')],text=True)
for sym in ('port_nsp_parse','port_nsp_build_connect','port_nsp_build_disconnect'):
    assert sym in syms,sym
print('M16 NSP guest integration/static symbols PASS')
