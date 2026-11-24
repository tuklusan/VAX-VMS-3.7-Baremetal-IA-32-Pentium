#!/usr/bin/env python3
"""Require the SYSQIO PCB transaction entry points in resident SYS.EXE."""
import subprocess
import sys

image = sys.argv[1] if len(sys.argv) > 1 else "build/sys.exe"
result = subprocess.run(["nm", "-g", image], check=True,
                        capture_output=True, text=True)
symbols = {}
for line in result.stdout.splitlines():
    fields = line.split()
    if len(fields) >= 3:
        symbols[fields[-1]] = fields[-2]
required = {
    "port_v37_sysqio_begin",
    "port_v37_sysqio_prepare_irp",
    "port_v37_sysqio_process_bind_pcb",
    "port_v37_ccb_resolve",
}
missing = sorted(required - symbols.keys())
if missing:
    raise SystemExit("SYS.EXE missing SYSQIO symbols: " + ", ".join(missing))
non_text = sorted(name for name in required if symbols[name].upper() not in ("T",))
if non_text:
    raise SystemExit("SYS.EXE SYSQIO entry points are not resident text: " +
                     ", ".join(non_text))
print("Resident SYS.EXE contains SYSQIO begin, IRP preparation and PCB binding; live service dispatch remains incomplete")
