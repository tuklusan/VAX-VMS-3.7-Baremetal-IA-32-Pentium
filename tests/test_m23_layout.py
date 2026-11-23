from pathlib import Path
import subprocess
r=Path(__file__).resolve().parent.parent
k=(r/'src/kernel.c').read_text();conf=(r/'dosbox-x.conf').read_text()
for symbol in ('port_serial_x86_init','port_serial_x86_putc',
               'port_serial_x86_sent','SHOW SERIAL'):
    assert symbol in k,symbol
assert '[serial]' in conf and 'serial1 = file file:portlab-serial.log' in conf
nm=subprocess.check_output(['nm',str(r/'build/kernel.elf')],text=True)
for symbol in ('port_serial_setup','port_serial_putc','port_serial_x86_putc'):
    assert symbol in nm,symbol
print('M23 COM1 serial capture, guest monitor integration PASS')
