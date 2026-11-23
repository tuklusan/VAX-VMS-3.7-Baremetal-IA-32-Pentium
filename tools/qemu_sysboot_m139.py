#!/usr/bin/env python3
"""One real native IA-32 QEMU boot; verify DPL0 software IPL4 dispatch.

Only source-derived loaded SYS.EXE PFLVEC/INTSTKLM records and an incomplete-INIT stop
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
        print('QEMU M139: system QEMU not installed',file=sys.stderr)
        return 77
    cmd=[machine,'-display','none','-monitor','none','-serial','stdio',
         '-m','128','-boot','a','-no-reboot','-accel','tcg',
         '-drive','if=floppy,format=raw,file='+str(ROOT/'vms37lab.ima')+',snapshot=on',
         '-drive','if=ide,index=0,media=disk,format=raw,file='+str(ROOT/'portdisk.img')+',snapshot=on']
    p=subprocess.Popen(cmd,stdin=subprocess.DEVNULL,stdout=subprocess.PIPE,
                       stderr=subprocess.PIPE,bufsize=0)
    observed=bytearray()
    expected=[
        b'SYSBOOT: Files-11 SYS.EXE normalized 32-bit retrieval map stored.',
        b'SYSBOOT: Files-11 SYS.EXE native image loaded.',
        b'INIT: loaded SYS.EXE initialized real IRP/LRP/SRP lookaside headers.',
        b'SYSBOOT: Files-11 SYSLOA386.EXE native image loaded.',
        b'INIT: SYS.EXE linked live SYSLOA386 processor vectors.',
        b'SYSBOOT: Files-11 INILOA.EXE native image loaded.',
        b'INIT: INILOA IOMAP executed on real IA-32 boot adapter.',
        b'INIT: loaded SYS.EXE built lock ID, resource hash and deadlock bitmap tables.',
        b'INIT: loaded SYS.EXE initialized SYSTEM and NULL PCB/sequence vectors.',
        b'INIT: loaded SYS.EXE initialized process header vector and swapper PTE records.',
        b'INIT: loaded SYS.EXE initialized modified-page writer PTE/index map.',
        b'INIT: loaded SYS.EXE allocated unbound pagefile pointer vector.',
        b'INIT: loaded SYS.EXE recorded real IA-32 TSS privilege stack and lock depth.',
        b'INIT: loaded SYS.EXE compacted real Files-11 SYS.EXE BOO$ WCB map.',
        b'INIT: SYS.EXE staged MB/NL/OP DPT headers (driver images not linked).',
        b'INIT: loaded SYS.EXE executed original NLDRIVER read/write FDT (IOC pending).',
        b'INIT: SYS.EXE staged original DYN$C_LOG=11 records (logical service pending).',
        b'INIT: SYS.EXE selected registered ATA boot adapter (DDB/UCB pending).',
        b'INIT: SYS.EXE staged DQDRIVER 12 source FDT masks (disk DDT and ACP unbound).',
        b'INIT: SYS.EXE linked NL$DDT DDTAB/FUNCTAB to NLA0 (other drivers absent).',
        b'INIT: SYS.EXE DQ READPBLK fetched actual ATA Files-11 LBN1 (disk DDT/ACP not linked).',
        b'INIT: SYS.EXE DQ READPBLK fetched real ATA LBN0 in second privileged IRP (no public QIO).',
        b'INIT: SYS.EXE DQ READPBLK verified real ATA INDEXF.SYS FH2 in third privileged IRP (ACP unbound).',
        b'INIT: SYS.EXE DQ READPBLK verified real ATA MFD FID 4,4,0 via INDEXF.SYS extent map (ACP unbound).',
        b'INIT: SYS.EXE native FIL$FINDFILID traversed real MFD and SYSEXE.DIR to SYS.EXE FID 11 (ACP unbound).',
        b'INIT: SYS.EXE FIN_DATABASE staged DQA0 names and resident SYS$DISK/SYS$SYSDEVICE (logical lookup pending).',
        b'INIT: SYS.EXE LOG$TRANSLATE resolved original SYS$DISK and SYS$SYSDEVICE names (system bootstrap only).',
        b'INIT: SYS.EXE executed console XON/XOFF/STOP/RESUME transitions (TT driver absent).',
        b'INIT: SYS.EXE IOC device search resolved NLA0 with native null DDT (others unbound).',
        b'INIT: SYS.EXE IOC$SEARCHDEV resolved SYS$DISK/SYS$SYSDEVICE to unbound DQA0 and NLA0 to linked DDT (no $ASSIGN).',
        b'INIT: SYS.EXE staged source DQ DPTAB/DDTAB and 136-byte FUNCTAB (all disk actions unbound).',
        b'INIT: SYS.EXE RELOCDRV IOC$INITDRV applied source DQ DPT_STORE to private DQA0 records (DDT/CRB unbound).',
        b'INIT: resident IOC$RELOC_DDT source DDT/FDT relocation verified (private only; DQA0 unbound).',
        b'INIT: SYS.EXE NLDRIVER completed two IRPs via linked DDT/FDT and EXE$FINISHIOC (IPL4 requested).',
        b'INIT: SYS.EXE IOCIOPOST serviced two NLA IRPs through IA-32 CPL0 software INT 0x90 (no process AST).',
        b'INIT: loaded SYS.EXE resident entry executed and verified.',
        b'INIT: linked SYSLOA386 read real IDT, CR0, CR3, TSS, PIT and Files-11 boot home.',
        b'INIT: actual IRQ0 advanced resident SYS.EXE VMS clock.',
        b'INIT stopped: remaining original initialization dependencies incomplete.']
    failures=[
        b'SYSBOOT: SYS.EXE retrieval map invalid.',
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
        b'INIT: resident SYS.EXE modified-page writer record validation failed.',
        b'INIT: resident SYS.EXE interrupt stack depth setup failed.',
        b'INIT: resident SYS.EXE SYSWCB map overlay validation failed.',
        b'INIT: resident SYS.EXE pointer vector setup failed.',
        b'INIT: resident SYS.EXE logical record staging failed.',
        b'INIT: resident SYS.EXE DPT prologue staging failed.',
        b'INIT: resident SYS.EXE null driver FDT validation failed.',
        b'INIT: resident SYS.EXE boot adapter scan failed.',
        b'INIT: resident source DQ FDT staging validation failed.',
        b'INIT: resident DQ physical boot READPBLK failed.',
        b'INIT: resident DQ physical LBN0 READPBLK failed.',
        b'INIT: resident SYS.EXE INDEXF.SYS physical FH2 read failed.',
        b'INIT: resident SYS.EXE MFD FH2 indexed physical read failed.',
        b'INIT: resident SYS.EXE Files-11 MFD/SYSEXE directory traversal failed.',
        b'INIT: resident FIN_DATABASE source boot DDB/logical record staging failed.',
        b'INIT: resident source LOGNAMSUB bootstrap translation failed.',
        b'INIT: SYS.EXE DEVICEDAT DDB/UCB prelink validation failed.',
        b'INIT: resident CONINTDSP source port action checks failed.',
        b'INIT: resident SYS.EXE physical IOC name search validation failed.',
        b'INIT: resident IOC logical to physical boot name lookup failed.',
        b'INIT: resident DQ DDT source table staging failed.',
        b'INIT: resident RELOCDRV original DQ INITDRV table failed.',
        b'INIT: resident SYS.EXE NLDRIVER to IOC post queue failed.',
        b'INIT: resident private IOC$RELOC_DDT failed.',
        b'INIT: resident IOCIOPOST source kernel end-action processing failed.',
        b'INIT: resident IOCIOPOST native software interrupt failed.',
        b'INIT: SYS.EXE resident initialization failed.',
        b'INIT: SYS.EXE PIT clock handoff failed.',
        b'INIT: live SYS.EXE PIT clock did not advance.',
        b'INIT: IA-32 privilege transition stack is unprotected.',
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
        # Preserve COM1 bytes exactly; QEMU stderr is a separate diagnostic.
        qemu_stderr=p.stderr.read(8192)
        serial_path=ROOT/'qemu-m139-serial.log'
        serial_path.write_bytes(observed)
        # Always retain the real transcript, including failed QEMU attempts.
        # GitHub run attempts get distinct keys; local runs get UTC keys.
        ci_run=os.environ.get('GITHUB_RUN_ID')
        ci_attempt=os.environ.get('GITHUB_RUN_ATTEMPT','1')
        identifier=(ci_run + ('-attempt'+ci_attempt if ci_attempt!='1' else '')
                    if ci_run else 'local-'+datetime.now(timezone.utc).strftime('%Y%m%dT%H%M%S%fZ'))
        run_url=('https://github.com/tuklusan/OpticOS/actions/runs/'+ci_run
                 if ci_run else '')
        capture(ROOT,serial_path,139,identifier,run_url,
                'harness-pass' if passed else 'harness-fail')
    if passed:
        print('QEMU M139: resident source DDT/FDT relocation in private SYS.EXE memory PASS')
        return 0
    print('QEMU M139: real guest execution or source-order check failed',file=sys.stderr)
    print(observed[-8192:].decode('ascii','replace'),file=sys.stderr)
    print(qemu_stderr.decode('ascii','replace'),file=sys.stderr)
    return 1
if __name__=='__main__':raise SystemExit(main())
