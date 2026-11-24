"""Pack the native PC BOO$BLOCK and the current executable."""
from pathlib import Path
from struct import pack, unpack_from
ROOT = Path(__file__).resolve().parent
boot = (ROOT/'build'/'boot.bin').read_bytes()
kernel = (ROOT/'build'/'kernel.bin').read_bytes()
assert len(boot) == 512 and boot[510:] == b'\x55\xaa', 'bad BIOS boot sector'
assert 0 < len(kernel) <= 260 * 512, f'kernel needs too many boot blocks ({len(kernel)} bytes)'
boot = bytearray(boot)
marker = boot.find(b'VMSB')
assert marker > 0 and boot.find(b'VMSB', marker+1) == -1 and marker+8 <= 510, 'missing boot control fields'
block_count = (len(kernel)+511)//512
boot[marker+4:marker+6] = pack('<H',block_count)
assert unpack_from('<H',boot,marker+6)[0] == 1, 'invalid starting boot LBN'
image = bytearray(1474560)
image[:512] = boot
image[512:512+len(kernel)] = kernel
(ROOT/'vms37lab.ima').write_bytes(image)
print(f'Floppy image: 1,474,560 bytes; stage2: {len(kernel):,} bytes ({block_count} blocks)')
