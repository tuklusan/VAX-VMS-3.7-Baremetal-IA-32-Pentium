"""Mutation checks: stage-1 interpreter must reject broken CPU handoffs."""
from pathlib import Path
import sys
r=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(r))
from tools.boot16_probe import Boot16, verify
image=(r/'vms37lab.ima').read_bytes()
verify(image)
boot=image[:512]

# A wrong CR0.PE immediate cannot be treated as a valid kernel transfer.
cr0=boot.find(bytes.fromhex('66 83 c8 01 0f 22 c0'))
assert cr0>=0
malformed=bytearray(image);malformed[cr0+3]=0
try:
    Boot16(bytes(malformed)).run()
except AssertionError:pass
else:raise AssertionError('CR0.PE mutation not detected')

# A wrong protected-mode code selector must abort the handoff model.
end=boot.find(bytes.fromhex('ea 00 00 01 00 08 00'))
assert end>=0
malformed=bytearray(image);malformed[end+5]=0x10
try:
    Boot16(bytes(malformed)).run()
except AssertionError:pass
else:raise AssertionError('far jump CS mutation not detected')

# A damaged kernel entry is rejected even though CHS and DMA remain valid.
malformed=bytearray(image);malformed[512]=0x90
try:
    Boot16(bytes(malformed)).run()
except AssertionError:pass
else:raise AssertionError('unbootable kernel entry not detected')
# The first-stage metadata is read by the assembled boot code. A missing
# or excessive length and a bad LBN must stop before any kernel transfer.
marker=boot.index(b'VMSB')
for field, bad in ((4,b'\x00\x00'),(4,b'\xfc\x00'),
                   (6,b'\x00\x00'),(6,b'\x40\x0b')):
    malformed=bytearray(image)
    malformed[marker+field:marker+field+2]=bad
    sim=Boot16(bytes(malformed)).run()
    assert sim.halted and not sim.reached_switch and not sim.reads
print('M75 BOOTBLOCK size/LBN checks and protected-mode mutations PASS')
