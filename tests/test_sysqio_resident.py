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
    "port_v37_ccb_find_free",
    "port_v37_ccb_assign_local",
    "port_exe_user_access_init",
    "port_user_range_valid",
    "port_user_copy_from",
    "port_user_copy_to",
}
missing = sorted(required - symbols.keys())
if missing:
    raise SystemExit("SYS.EXE missing SYSQIO symbols: " + ", ".join(missing))
non_text = sorted(name for name in required if symbols[name].upper() not in ("T",))
if non_text:
    raise SystemExit("SYS.EXE SYSQIO entry points are not resident text: " +
                     ", ".join(non_text))
print("Resident SYS.EXE contains SYSQIO, source CCB, and translated caller-memory helpers; live service dispatch remains incomplete")
