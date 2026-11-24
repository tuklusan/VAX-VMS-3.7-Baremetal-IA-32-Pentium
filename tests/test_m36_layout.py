"""M36 UART RX: current cooked/raw console paths; no stale old-line matching."""
from pathlib import Path
import subprocess

r = Path(__file__).resolve().parents[1]
s = (r/'src/kernel.c').read_text()
serial = (r/'src/vms_serial.c').read_text()
assert 'static char getkey_mode(int preserve_case)' in s
assert 'preserve_case?port_serial_x86_getc_raw():port_serial_x86_getc()' in s
assert 'if(serial>=0)return (char)serial;' in s
assert 'if ((inb(0x64) & 1) == 0) { decnet_service(); continue; }' in s
assert 'int port_serial_getc(' in serial
names = subprocess.check_output(['nm', str(r/'build/kernel.elf')], text=True)
assert 'port_serial_x86_getc' in names
assert 'port_serial_x86_getc_raw' in names
assert len((r/'build/kernel.bin').read_bytes()) <= 260*512
print('M36 raw/cooked UART console poll and fallback linked PASS')
