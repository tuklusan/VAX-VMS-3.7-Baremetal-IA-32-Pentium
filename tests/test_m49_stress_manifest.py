#!/usr/bin/env python3
"""Ensure M49 stress assertions cannot silently drift from the real guest ABI."""
import importlib.util
from pathlib import Path
root=Path(__file__).resolve().parent.parent
spec=importlib.util.spec_from_file_location('qemu_smoke',root/'tools/qemu_smoke.py')
module=importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)
assert len(module.CHECKS)==32
assert len(module.STRESS_CHECKS)==76
assert sum(c=='TEST DEMAND' for c,_ in module.STRESS_CHECKS)==12
assert sum(c=='SHOW MEMORY' for c,_ in module.STRESS_CHECKS)==12
assert sum(c=='TEST ASTDEL' for c,_ in module.STRESS_CHECKS)==12
assert sum(c=='TEST POSTEF' for c,_ in module.STRESS_CHECKS)==12
assert sum(c=='TEST MUTEX' for c,_ in module.STRESS_CHECKS)==4
assert sum(c=='TEST MUTEXCPL3' for c,_ in module.STRESS_CHECKS)==4
assert len(set(x for x,_ in module.STRESS_CHECKS))==11
for command,expected in module.STRESS_CHECKS:
    assert any(c==command for c,_ in module.CHECKS)
    if command!='SHOW MEMORY':
        assert expected.decode('ascii') in (root/'src/kernel.c').read_text()
    else:
        assert 'Physical 4K frames free=' in (root/'src/kernel.c').read_text()
assert all(x=='Physical 4K frames free=0x00000200' for c,e in module.STRESS_CHECKS if c=='SHOW MEMORY' for x in [e.decode('ascii')])
print('M110: 32 baseline + 76 repeated guest checks, manifest PASS')
