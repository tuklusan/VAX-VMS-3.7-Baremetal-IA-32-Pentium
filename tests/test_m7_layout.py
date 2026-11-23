"""Linked-image ABI and page-guard static checks (not guest runtime)."""
from pathlib import Path
import subprocess
import re
r=Path(__file__).resolve().parent.parent
symbols={}
for line in subprocess.check_output(['nm','-n',str(r/'build/kernel.elf')],text=True).splitlines():
    m=re.match(r'^([0-9a-fA-F]+)\s+\w\s+(\S+)$',line)
    if m: symbols[m[2]]=int(m[1],16)
for n in ['task_kernel_stack','port_kstack_contains_frame','port_kstack_intact',
          'port_pmap_guard_page','arch_kstack_protected',
          'arch_kstack_checks','__bss_end']:
    assert n in symbols, n
assert symbols['task_kernel_stack']%4096==0
assert symbols['__bss_end']<0x90000
c=(r/'src/arch.c').read_text()
assert 'port_pmap_guard_page(table,port_kstack_guard(&task_kernel_stack[i]))' in c
assert 'guard_kernel_stacks(low_page_table)' in c
assert 'guard_kernel_stacks(task_page_table[i])' in c
assert 'tss.esp0=(uint32_t)port_kstack_top(&task_kernel_stack[task])' in c
assert 'port_kstack_contains_frame(&task_kernel_stack[task]' in c
assert 'kernel_stack_faults' in c
assert 'monitor_kernel_stack();' in c
assert 'TEST KSTACK' in (r/'src/kernel.c').read_text()
print('M7 linked layout PASS: two guarded stacks, TSS switching, bounds')
