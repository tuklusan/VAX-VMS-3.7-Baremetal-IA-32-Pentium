"""Verify the real-mode fast-A20 gate and pre-paging alias probe wiring."""
from pathlib import Path
import subprocess
import runpy
r=Path(__file__).resolve().parents[1]
boot=(r/'build/boot.bin').read_bytes()
assert len(boot)==512 and boot[-2:]==b'\x55\xaa'
# 16-bit opcodes: IN AL,92; OR AL,2; AND AL,FE (fast reset off); OUT 92,AL.
gate=bytes.fromhex('e4 92 0c 02 24 fe e6 92')
assert boot.count(gate)==1
assert boot.index(gate)<boot.find(b'\x0f\x01\x16')  # LGDT comes after gate
assert b'Disk read failed.\r\n\x00' in boot
s=(r/'src/arch.c').read_text()
for stmt in ['arch_phys_read8(0x00000500u)', 'arch_phys_read8(0x00100500u)',
             'arch_phys_write8(0x00000500u,0x5au)',
             'arch_phys_write8(0x00100500u,0xa5u)',
             'arch_phys_write8(0x00100500u,old_high)',
             'arch_phys_write8(0x00000500u,old_low)']:
    assert stmt in s, stmt
init=s[s.index('void arch_init(void) {'):]
assert init.index('arch_verify_a20()')<init.index('arch_install_gdt()')
assert init.index('if (!a20_ready) return')<init.index('arch_install_gdt()')
assert '0x00100000u' in (r/'src/vms_phys.h').read_text()
k=(r/'src/kernel.c').read_text()
assert '"SHOW A20"' in k and 'arch_a20_ready()' in k
assert 'System initialization stopped: PC memory check failed.' in k
assert dict(runpy.run_path(str(r/"tools/qemu_smoke.py"))["CHECKS"])["SHOW A20"] == b"A20 enabled and verified."
nm=subprocess.check_output(['nm',str(r/'build/kernel.elf')],text=True)
assert 'arch_a20_ready' in nm
print('M41 A20 fast gate, safe reset bit, verified nonaliasing, fatal path PASS')
