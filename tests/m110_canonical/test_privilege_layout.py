"""Static assertions for the M3 ring-transition image; not a runtime emulator test."""
from pathlib import Path
import re
import subprocess

root = Path(__file__).resolve().parent.parent
elf = root / 'build/kernel.elf'
binary = (root / 'build/kernel.bin').read_bytes()
output = subprocess.check_output(['nm', '-n', str(elf)], text=True)
symbols = {}
for line in output.splitlines():
    match = re.match(r'^([0-9a-fA-F]+)\s+\w\s+(\S+)$', line)
    if match:
        symbols[match.group(2)] = int(match.group(1), 16)

required = ['arch_enter_ring3', 'arch_ring3_resume', 'arch_load_tr',
            'arch_user_basic', 'arch_user_fault', 'arch_user_ast',
            'arch_user_fault_site', 'arch_user_fault_after',
            'arch_user_code_begin', 'arch_user_code_end', 'arch_isr_128',
            'arch_run_user_test', 'arch_run_user_ast', 'port_gdt',
            'user_stack', 'task_b_stack', 'page_directory', 'low_page_table',
            '__bss_start', '__bss_end', 'arch_isr_table']
assert all(name in symbols for name in required), sorted(set(required) - symbols.keys())

# User text must occupy a dedicated page so enabling its PTE U/S bit doesn't
# expose the GDT or kernel global data. This caught a real early M3 bug.
user_start = symbols['arch_user_code_begin']
user_end = symbols['arch_user_code_end']
assert user_start % 4096 == 0
assert user_start < symbols['arch_user_fault_site'] < symbols['arch_user_fault_after'] < user_end
assert user_end <= user_start + 4096, 'user text crossed a page boundary'
assert symbols['port_gdt'] >= user_start + 4096, 'GDT in user-accessible page'
assert symbols['__bss_start'] >= user_start + 4096
assert symbols['user_stack'] % 4096 == 0
assert symbols['task_b_stack'] % 4096 == 0
assert symbols['task_b_stack'] != symbols['user_stack']
assert symbols['page_directory'] % 4096 == 0
assert symbols['low_page_table'] % 4096 == 0
assert symbols['__bss_end'] < 0x90000  # don't overwrite initial 0x90000 stack

# IDT vector table now includes the DPL3 syscall entry at index 34.
table = symbols['arch_isr_table'] - 0x10000
assert table >= 0
assert table + 35*4 <= len(binary)
for idx in range(35):
    target = int.from_bytes(binary[table+4*idx:table+4*idx+4], 'little')
    assert 0x10000 <= target < 0x10000 + len(binary)
assert int.from_bytes(binary[table+34*4:table+35*4], 'little') == symbols['arch_isr_128']

# Encoded instruction sites must be plausible before running risky #PF tests.
site = symbols['arch_user_fault_site'] - 0x10000
assert binary[site:site+2] == b'\xc7\x05', 'expected MOV imm32, absolute32 at probe'
assert binary[site+2:site+6] == (0x10000).to_bytes(4, 'little')
assert symbols['arch_user_fault_after'] == symbols['arch_user_fault_site'] + 10
assert all(x < 0x100000 for x in [user_end, symbols['__bss_end']])
print('Privilege layout PASS: separate user page, TSS/stack symbols, 35 trap stubs, #PF site')
