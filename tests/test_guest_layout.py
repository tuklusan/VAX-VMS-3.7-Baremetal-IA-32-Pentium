"""Static IA-32 ELF and diagnostic-site checks (not an emulator run)."""
from pathlib import Path
import re
import subprocess

root=Path(__file__).resolve().parent.parent
binary=(root/'build/kernel.bin').read_bytes()
nm=subprocess.check_output(['nm','-n',str(root/'build/kernel.elf')],text=True)
symbols={m.group(2):int(m.group(1),16) for line in nm.splitlines()
         if (m:=re.match(r'^([0-9a-fA-F]+)\s+\S\s+(\S+)$',line))}
required=['_start','arch_init','arch_trap','arch_isr_table',
          'arch_probe_int3','arch_probe_int3_after','arch_probe_ud2',
          'vms_ast_dispatch_one','vms_ast_enqueue','port_condition_raise']
for name in required:
    assert name in symbols, f'missing kernel symbol: {name}'
assert symbols['_start']==0x10000

def offset(name):
    pos=symbols[name]-0x10000
    assert 0<=pos<len(binary)
    return pos

assert binary[offset('arch_probe_int3')]==0xcc, 'INT3 test instruction missing'
assert binary[offset('arch_probe_ud2'):offset('arch_probe_ud2')+2]==b'\x0f\x0b', 'UD2 test instruction missing'
assert symbols['arch_probe_int3_after']==symbols['arch_probe_int3']+1
# A valid table of addresses for the 35 installed vector entry points.
table=offset('arch_isr_table')
assert table+35*4<=len(binary)
for idx in range(35):
    target=int.from_bytes(binary[table+idx*4:table+(idx+1)*4],'little')
    assert 0x10000<=target<0x10000+len(binary), f'IDT stub {idx} points outside loaded payload'
print('Guest layout checks PASS: entry, AST/condition symbols, diagnostic opcodes, 35 vector stubs')
