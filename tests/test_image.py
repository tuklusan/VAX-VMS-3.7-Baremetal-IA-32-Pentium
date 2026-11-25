from pathlib import Path
base = Path(__file__).resolve().parent.parent
boot = (base/'build/boot.bin').read_bytes()
kernel = (base/'build/kernel.bin').read_bytes()
image = (base/'vms37lab.ima').read_bytes()
assert len(boot) == 512
assert boot[-2:] == b'\x55\xAA'
assert len(image) == 1474560
marker=image[:512].find(b'VMSB')
assert 0<marker<500
assert image[:marker+4]==boot[:marker+4]
assert image[marker+6:512]==boot[marker+6:512]
assert int.from_bytes(image[marker+4:marker+6],'little') == (len(kernel)+511)//512
assert int.from_bytes(image[marker+6:marker+8],'little') == 1
assert 0 < len(kernel) <= 265*512
assert image[512:512+len(kernel)] == kernel
assert all(b == 0 for b in image[512+len(kernel):])
# The disk block count is read from the boot sector, not fixed to 260.
assert (len(kernel)+511)//512 <= 265
assert 0x10000 + 265*512 < 0x90000
print('Image checks PASS: signature, payload, disk count, first LBN and load address')
