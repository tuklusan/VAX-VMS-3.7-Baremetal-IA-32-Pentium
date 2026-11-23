"""Kernel RAM-window probe must precede any paging/allocator setup."""
from pathlib import Path
import subprocess
import runpy
r=Path(__file__).resolve().parents[1]
a=(r/'src/arch.c').read_text()
k=(r/'src/kernel.c').read_text()
q=(r/'tools/qemu_smoke.py').read_text()
start=a[a.index('void arch_init(void) {'):a.index('static int port_ranges_overlap')]
assert start.index('arch_verify_a20()') < start.index('port_ram_probe_3mib(&ram_bus)')
assert start.index('port_ram_probe_3mib(&ram_bus)') < start.index('arch_install_gdt()')
assert 'if (!ram_window_ready) return;' in start
assert 'if(!arch_a20_ready()||!arch_ram_window_ready())' in k
assert 'System initialization stopped: PC memory check failed.' in k
assert 'SHOW RAM' in k and '3MiB RAM window alias probe PASS.' in k
assert dict(runpy.run_path(str(r/"tools/qemu_smoke.py"))["CHECKS"])["SHOW RAM"] == b"3MiB RAM window alias probe PASS."
symbols=subprocess.check_output(['nm',str(r/'build/kernel.elf')],text=True)
assert 'port_ram_probe_3mib' in symbols and 'arch_ram_window_ready' in symbols
print('M44 early physical RAM alias guard linked before allocator PASS')
