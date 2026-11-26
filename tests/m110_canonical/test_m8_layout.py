"""M8 linked-image/static checks; not a substitute for guest boot testing."""
from pathlib import Path
import re, subprocess
r=Path(__file__).resolve().parent.parent
s=(r/'src/arch.c').read_text()
k=(r/'src/kernel.c').read_text()
a=(r/'src/arch.s').read_text()
for symbol in ['arch_run_demand_test','arch_demand_faults','zero_physical_frame',
               'port_vmem_fault','port_vmem_release','invlpg']:
    assert symbol in s, symbol
for cmd in ['TEST DEMAND','SHOW VM']:
    assert cmd in k
for name in ['arch_user_demand_a','arch_user_demand_b']:
    assert name in a
symbols={}
for line in subprocess.check_output(['nm','-n',str(r/'build/kernel.elf')],text=True).splitlines():
    m=re.match(r'^([0-9a-fA-F]+)\s+\w\s+(\S+)$',line)
    if m:symbols[m[2]]=int(m[1],16)
for n in ['arch_user_demand_a','arch_user_demand_b','arch_user_code_end',
          'port_vmem_fault','__bss_end']:
    assert n in symbols,n
assert symbols['arch_user_demand_a'] < symbols['arch_user_code_end']
assert symbols['arch_user_demand_b'] < symbols['arch_user_code_end']
assert symbols['__bss_end']<0x9e000
print('M8 linked layout PASS: user demand faults, native handler, reclaim')
