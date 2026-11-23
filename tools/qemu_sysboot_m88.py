#!/usr/bin/env python3
"""One real QEMU PC, real ODS-2 boot, deterministic source-order stop.

No fabricated SYSTEM login or simulated VMS boot is accepted.  The
expected failure is the missing INILOA executable after SYS.EXE loaded.
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
    executable=shutil.which('qemu-system-i386') or shutil.which('qemu-system-x86_64')
    if not executable:
        print('QEMU M88: NOT RUN (no QEMU system emulator installed)',file=sys.stderr)
        return 77
    command=[executable,'-display','none','-monitor','none',
             '-serial','stdio','-m','128','-boot','a','-no-reboot',
             '-accel','tcg',
             '-drive','if=floppy,format=raw,file='+str(ROOT/'vms37lab.ima')+',snapshot=on',
             '-drive','if=ide,index=0,media=disk,format=raw,file='+str(ROOT/'portdisk.img')+',snapshot=on']
    p=subprocess.Popen(command,stdin=subprocess.DEVNULL,stdout=subprocess.PIPE,
                       stderr=subprocess.PIPE,bufsize=0)
    log=bytearray()
    deadline=time.monotonic()+60
    expected=(b'SYSBOOT: Files-11 SYS.EXE native image loaded.',
              b'-F-Unable to locate INILOA.EXE')
    fail=(b'-F-Unable to locate SYS.EXE',
          b'SYSBOOT: SYS.EXE native image validation or loading failed.',
          b'PORTLAB> ',b'$ ',b'SYSTEM login successful')
    ok=False
    try:
        while time.monotonic()<deadline:
            if any(m in log for m in fail):break
            if all(m in log for m in expected):ok=True;break
            if p.poll() is not None:break
            ready,_,_=select.select([p.stdout],[],[],0.25)
            if ready:
                chunk=os.read(p.stdout.fileno(),4096)
                if not chunk:break
                log.extend(chunk)
    finally:
        p.terminate()
        try:p.wait(timeout=3)
        except subprocess.TimeoutExpired:
            p.kill();p.wait()
        log.extend(b'\n=== QEMU stderr ===\n')
        log.extend(p.stderr.read(8192))
        (ROOT/'qemu-m88-serial.log').write_bytes(log)
    if ok:
        print('QEMU M88: native SYS.EXE Files-11 load reached; INILOA absent as expected PASS')
        return 0
    print('QEMU M88: boot failed before verified SYS.EXE load or unexpected success',file=sys.stderr)
    print(log[-4096:].decode('ascii','replace'),file=sys.stderr)
    return 1
if __name__=='__main__':raise SystemExit(main())
