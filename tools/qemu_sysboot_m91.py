#!/usr/bin/env python3
"""One native IA-32 QEMU guest; require executed source-derived images.

This checks running code in SYS.EXE and INILOA.EXE, not just printed boot
load messages. The expected stop is missing the remaining original INIT
services, not a manufactured SYSTEM login.
"""
import os
from pathlib import Path
import select
import shutil
import subprocess
import sys
import time
ROOT=Path(__file__).resolve().parents[1]

def main():
    machine=shutil.which('qemu-system-i386') or shutil.which('qemu-system-x86_64')
    if not machine:
        print('QEMU M91: no system QEMU installed',file=sys.stderr)
        return 77
    cmd=[machine,'-display','none','-monitor','none','-serial','stdio',
         '-m','128','-boot','a','-no-reboot','-accel','tcg',
         '-drive','if=floppy,format=raw,file='+str(ROOT/'vms37lab.ima')+',snapshot=on',
         '-drive','if=ide,index=0,media=disk,format=raw,file='+str(ROOT/'portdisk.img')+',snapshot=on']
    p=subprocess.Popen(cmd,stdin=subprocess.DEVNULL,stdout=subprocess.PIPE,
                       stderr=subprocess.PIPE,bufsize=0)
    observed=bytearray()
    expected=[
       b'SYSBOOT: Files-11 SYS.EXE native image loaded.',
       b'SYSBOOT: Files-11 INILOA.EXE native image loaded.',
       b'INIT: INILOA IOMAP executed on real IA-32 boot adapter.',
       b'INIT: loaded SYS.EXE resident entry executed and verified.',
       b'INIT: actual IRQ0 advanced resident SYS.EXE VMS clock.',
       b'INIT stopped: remaining original initialization dependencies incomplete.']
    reject=[b'SYSBOOT: SYS.EXE native image validation or loading failed.',
            b'SYSBOOT: INILOA.EXE image validation or loading failed.',
            b'INIT: INILOA adapter initialization failed.',
            b'INIT: SYS.EXE resident initialization failed.',
            b'INIT: hardware system clock is unavailable.',
            b'INIT: SYS.EXE PIT clock handoff failed.',
            b'INIT: live SYS.EXE PIT clock did not advance.',
            b'PORTLAB> ',b'Login successful',b'Login: ',b'\n$ ']
    passed=False
    try:
        until=time.monotonic()+80
        while time.monotonic()<until:
            if any(x in observed for x in reject):break
            if all(x in observed for x in expected):
                # Require exact source-stage order, not just the substrings.
                positions=[observed.index(x) for x in expected]
                passed=positions==sorted(positions)
                break
            if p.poll() is not None:break
            readable,_,_=select.select([p.stdout],[],[],0.25)
            if readable:
                chunk=os.read(p.stdout.fileno(),4096)
                if not chunk:break
                observed.extend(chunk)
    finally:
        p.terminate()
        try:p.wait(timeout=3)
        except subprocess.TimeoutExpired:p.kill();p.wait()
        observed.extend(b'\n=== QEMU stderr ===\n')
        observed.extend(p.stderr.read(8192))
        (ROOT/'qemu-m91-serial.log').write_bytes(observed)
    if passed:
        print('QEMU M91: guest executed real Files-11 INILOA and SYS.EXE entry code PASS')
        return 0
    print('QEMU M91: execution evidence absent or failed',file=sys.stderr)
    print(observed[-8192:].decode('ascii','replace'),file=sys.stderr)
    return 1
if __name__=='__main__':raise SystemExit(main())
