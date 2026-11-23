#!/usr/bin/env python3
"""Catch guest monitor/assertion drift before spending a QEMU CI run."""
import importlib.util
import re
from pathlib import Path

root = Path(__file__).resolve().parent.parent
spec = importlib.util.spec_from_file_location("qemu_smoke", root / "tools/qemu_smoke.py")
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)
code = (root / "src/kernel.c").read_text()
assert len(module.CHECKS) == 32
assert len(set(x for x, _ in module.CHECKS)) == len(module.CHECKS)
for command, text in module.CHECKS:
    assert re.search(r'\beq\(line,\s*"' + re.escape(command) + r'"\)', code), command
    if command.startswith('TEST '):
        assert text.decode('ascii') in code, command
for forbidden in ('readonly=on',):
    assert forbidden not in (root / 'tools/qemu_smoke.py').read_text()
print(f"M110 historic guest-monitor manifest: {len(module.CHECKS)} unique guest checks mapped to kernel PASS strings; QEMU snapshot disks PASS")
