"""M5+ linked-ELF and static ABI checks. Not an emulator execution test."""
from pathlib import Path
import subprocess
import re
root=Path(__file__).resolve().parent.parent
nm=subprocess.check_output(['nm','-n',str(root/'build/kernel.elf')],text=True)
syms={}
for line in nm.splitlines():
    m=re.match(r'([0-9a-fA-F]+)\s+\w\s+(\w+)',line)
    if m: syms[m[2]]=int(m[1],16)
for symbol in ['arch_user_preempt_a','arch_user_preempt_b','arch_user_auto_ast',
               'arch_user_isolation','arch_user_iso_fault_site',
               'arch_user_iso_fault_after','arch_user_clock_page',
               'clock_alias_table','task_page_directory','task_page_table',
               'arch_sched_preemptions','arch_run_preempt_test',
               'arch_run_isolation_test','arch_ring3_resume',
               'arch_user_return_eflags']:
    assert symbol in syms,symbol
user_start=syms['arch_user_code_begin'];user_end=syms['arch_user_code_end']
for name in ['arch_user_preempt_a','arch_user_preempt_b','arch_user_auto_ast',
             'arch_user_iso_fault_site']:
    assert user_start <= syms[name] < user_end,name
assert user_end <= user_start+4096
for name in ['arch_user_clock_page','clock_alias_table','task_page_directory',
             'task_page_table']:
    assert syms[name]%4096==0,name
assert syms['arch_user_clock_page'] != syms['task_b_stack']
assert syms['__bss_end'] < 0x9e000
asm=(root/'src/arch.s').read_text()
c=(root/'src/arch.c').read_text()
assert 'pushl arch_user_return_eflags' in asm and 'popfl' in asm
assert 'arch_user_auto_ast:' in asm and 'movl $6,%eax' in asm
assert 'int $0x80' in asm
assert 'arch_user_iso_fault_site:' in asm and 'task_b_stack' in asm
assert 'if (sched_active && sched_preempt && (f->cs&3u)==3u)' in c
assert 'switch_directory(task_page_directory[next])' in c
assert 'switch_directory(page_directory)' in c
assert 'copy_frame(&pcb->ast_resume,f)' in c
assert 'copy_frame(f,&task_pcb[cur].ast_resume)' in c
assert '*(volatile uint32_t *)0x400000u=ticks' in c
assert 'task_page_directory[i][0]=(uint32_t)(uintptr_t)task_page_table[i]|7u' in c
assert 'task_page_directory[i][1]=(uint32_t)(uintptr_t)clock_alias_table|3u' in c
print('M5 static layout PASS: preemption, isolated CR3, AST frame trampoline, clock alias, IF restore')
