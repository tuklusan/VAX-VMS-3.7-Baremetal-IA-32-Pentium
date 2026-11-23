from pathlib import Path
import subprocess
r=Path(__file__).resolve().parent.parent
k=(r/'src/kernel.c').read_text()
for term in ('POLL ROUTER','port_decnet_parse_router_hello',
             'port_decnet_adj_update','port_decnet_adj_expire',
             'port_decnet_adj_best','designated router','decnet_service()'):
    assert term in k
sym=subprocess.check_output(['nm',str(r/'build/kernel.elf')],text=True)
for name in ('port_decnet_parse_router_hello','port_decnet_adj_update',
             'port_decnet_adj_expire','port_decnet_adj_best'):
    assert name in sym
print('M15 adjacency static PASS: receive polling, designated router, timeout')
