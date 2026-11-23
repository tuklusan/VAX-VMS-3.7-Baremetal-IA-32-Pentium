#!/usr/bin/env python3
"""One native IA-32 QEMU boot; require source-derived SYS.EXE I/O packet queues.

Only source-derived image execution and a deliberate incomplete-INIT stop
are successful here. A fabricated login or an old monitor prompt is failure.
"""
import os
from pathlib import Path
import select
import shutil
import subprocess
import sys
import time
from datetime import datetime, timezone
from checkpoint_serial_logs import capture
ROOT=Path(__file__).resolve().parents[1]

def main():
    machine=shutil.which('qemu-system-i386') or shutil.which('qemu-system-x86_64')
    if not machine:
        print('QEMU M96: system QEMU not installed',file=sys.stderr)
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
        b'INIT: loaded SYS.EXE initialized real IRP/LRP/SRP lookaside headers.',
        b'SYSBOOT: Files-11 SYSLOA386.EXE native image loaded.',
        b'INIT: SYS.EXE linked live SYSLOA386 processor vectors.',
        b'SYSBOOT: Files-11 INILOA.EXE native image loaded.',
        b'INIT: INILOA IOMAP executed on real IA-32 boot adapter.',
        b'INIT: loaded SYS.EXE built lock ID, resource hash and deadlock bitmap tables.',
        b'INIT: loaded SYS.EXE initialized SYSTEM and NULL PCB/sequence vectors.',
        b'INIT: loaded SYS.EXE initialized process header vector and swapper PTE records.',
        b'INIT: loaded SYS.EXE resident entry executed and verified.',
        b'INIT: linked SYSLOA386 read real IDT, CR0, CR3, TSS, PIT and Files-11 boot home.',
        b'INIT: actual IRQ0 advanced resident SYS.EXE VMS clock.',
        b'INIT stopped: remaining original initialization dependencies incomplete.']
    failures=[
        b'-F-Unable to locate SYS.EXE',
        b'-F-Unable to locate usable SYSLOA386.EXE',
        b'-F-Unable to locate INILOA.EXE',
        b'SYSBOOT: SYSLOA386.EXE image validation or loading failed.',
        b'INIT: SYSLOA386 self-describing vector image invalid.',
        b'INIT: resident SYS.EXE SYSLOA386 vector linking failed.',
        b'INIT: SYS.EXE packet queue initialization failed.',
        b'INIT: resident SYS.EXE lock database setup failed.',
        b'INIT: resident SYS.EXE PCB sequence vectors failed.',
        b'INIT: resident SYS.EXE PHV and swap PTE record validation failed.',
        b'INIT: SYS.EXE resident initialization failed.',
        b'INIT: SYS.EXE PIT clock handoff failed.',
        b'INIT: live SYS.EXE PIT clock did not advance.',
        b'PORTLAB> ',b'Login successful',b'Login: ',b'\n$ ']
    passed=False
    try:
        until=time.monotonic()+80
        while time.monotonic()<until:
            if any(x in observed for x in failures):break
            if all(x in observed for x in expected):
                positions=[observed.index(x) for x in expected]
                passed=positions==sorted(positions)
                break
            if p.poll() is not None:break
            ready,_,_=select.select([p.stdout],[],[],0.25)
            if ready:
                chunk=os.read(p.stdout.fileno(),4096)
                if not chunk:break
                observed.extend(chunk)
    finally:
        p.terminate()
        try:p.wait(timeout=3)
        except subprocess.TimeoutExpired:p.kill();p.wait()
        observed.extend(b'\n=== QEMU stderr ===\n')
        observed.extend(p.stderr.read(8192))
        serial_path=ROOT/'qemu-m96-serial.log'
        serial_path.write_bytes(observed)
        # Always retain the real transcript, including failed QEMU attempts.
        # GitHub run attempts get distinct keys; local runs get UTC keys.
        ci_run=os.environ.get('GITHUB_RUN_ID')
        ci_attempt=os.environ.get('GITHUB_RUN_ATTEMPT','1')
        identifier=(ci_run + ('-attempt'+ci_attempt if ci_attempt!='1' else '')
                    if ci_run else 'local-'+datetime.now(timezone.utc).strftime('%Y%m%dT%H%M%S%fZ'))
        run_url=('https://github.com/tuklusan/OpticOS/actions/runs/'+ci_run
                 if ci_run else '')
        capture(ROOT,serial_path,96,identifier,run_url,
                'harness-pass' if passed else 'harness-fail')
    if passed:
        print('QEMU M96: SYS.EXE lock structures, IRP queues and live IRQ/IDT/CR3/TSS/Files-11 verified PASS')
        return 0
    print('QEMU M96: real guest execution or source-order check failed',file=sys.stderr)
    print(observed[-8192:].decode('ascii','replace'),file=sys.stderr)
    return 1
if __name__=='__main__':raise SystemExit(main())
