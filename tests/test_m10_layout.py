"""Native IA-32 ATA PIO layout and actual boot-device link checks."""
from pathlib import Path
import re
import subprocess
root=Path(__file__).resolve().parents[1]
k=(root/'src/kernel.c').read_text()
source=(root/'src/vms_ata.c').read_text()
adp=(root/'src/vms_ini_pc_bus.c').read_text()
for name in ['port_ata_x86_read28','native_ini_ata_read',
             'port_ini_pc_describe','loaded_iniloa']:
    assert name in k
for field in ['ATA_READ=0x20','ATA_DRQ=0x08','100000u','ATA_ALTSTATUS']:
    assert field in source
for name in ['port_ini_pc_probe','port_ini_pc_register',
             'PORT_INI_PC_ATA_PIO']:
    assert name in adp
script=(root/'tools/qemu_sysboot_m90.py').read_text()
assert "'if=ide,index=0,media=disk" in script
syms={}
for line in subprocess.check_output(['nm','-n',str(root/'build/kernel.elf')],text=True).splitlines():
    m=re.match(r'^([0-9A-Fa-f]+)\s+\S\s+(\S+)$',line)
    if m:syms[m[2]]=int(m[1],16)
for symbol in ['port_ata_x86_read28','port_ata_read28',
               'port_ini_pc_probe','port_ini_pc_register',
               'port_ini_pc_describe','__bss_end']:
    assert symbol in syms,symbol
assert syms['__bss_end']<0x9e000
print('M10 ATA native boot: protected PIO and INILOA driver registration PASS')
