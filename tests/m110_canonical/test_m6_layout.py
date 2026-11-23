"""M6 ABI and linked-binary static tests; not emulator runtime checks."""
from pathlib import Path
import re
import subprocess
root=Path(__file__).resolve().parent.parent
syms={}
for line in subprocess.check_output(['nm','-n',str(root/'build/kernel.elf')],text=True).splitlines():
    m=re.match(r'^([0-9a-fA-F]+)\s+\w\s+(\S+)$',line)
    if m: syms[m.group(2)]=int(m.group(1),16)
for name in ['arch_user_mem_a','arch_user_mem_b','arch_run_vm_test','task_pcb',
             'physical_pool','port_pcb_create','port_pcb_activate','port_pcb_exit',
             'port_phys_alloc','port_phys_free','arch_pcb_cr3','arch_vm_isolated']:
    assert name in syms,name
assert syms['arch_user_code_begin'] <= syms['arch_user_mem_a'] < syms['arch_user_code_end']
assert syms['arch_user_code_begin'] <= syms['arch_user_mem_b'] < syms['arch_user_code_end']
assert syms['arch_user_code_end'] <= syms['arch_user_code_begin']+4096
assert syms['__bss_end'] < 0x90000
c=(root/'src/arch.c').read_text()
asm=(root/'src/arch.s').read_text()
assert 'task_pcb[sched_current].active_ast' in c
assert 'task_page_table[i][virt>>12]=pa|7u' in c
assert 'port_phys_free(&physical_pool' in c
assert asm.count('0x00300000') >= 4
assert 'TEST PCB' in (root/'src/kernel.c').read_text()
print('M6 static checks PASS: PCB ABI, same-VA private frame mapping, reclaim')
