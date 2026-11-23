"""M4 build-level checks, not substitute for an emulator boot test."""
from pathlib import Path
import re
import subprocess
root=Path(__file__).resolve().parent.parent
sym={}
for line in subprocess.check_output(['nm','-n',str(root/'build/kernel.elf')],text=True).splitlines():
    m=re.match(r'([0-9a-fA-F]+)\s+\w\s+(\w+)',line)
    if m:sym[m.group(2)]=int(m.group(1),16)
for label in ('arch_user_task_a','arch_user_task_b','task_b_stack','task_pcb','arch_run_scheduler_test','arch_sched_sequence'):
    assert label in sym,label
assert sym['arch_user_code_begin'] < sym['arch_user_task_a'] < sym['arch_user_task_b'] < sym['arch_user_code_end']
assert sym['task_b_stack'] % 4096==0
assert sym['task_b_stack'] != sym['user_stack']
assert sym['arch_user_code_end'] <= sym['arch_user_code_begin']+4096
src=(root/'src/arch.c').read_text()
assert 'copy_frame(f,&new->frame)' in src
assert 'schedule_next(f,1)' in src and 'schedule_next(f,0)' in src
assert 'map_user_pages((uintptr_t)task_b_stack' in src
print('M4 scheduler layout PASS: CPL3 task text, two stacks, saved frames, yield/exit hooks')
