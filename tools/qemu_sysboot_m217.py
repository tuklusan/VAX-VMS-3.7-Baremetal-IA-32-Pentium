#!/usr/bin/env python3
"""One real native IA-32 QEMU boot; verify the 700-byte Files-11 chain read across DQA0 sectors.

The checked guest runs a private DDT-gated physical READPBLK and stops at
the still-incomplete INIT boundary. Login/DCL are not claimed.
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
    params_fixture=(len(sys.argv)==2 and sys.argv[1]=='--params-fixture')
    if len(sys.argv)>1 and not params_fixture:
        print('usage: qemu_sysboot_m217.py [--params-fixture]',file=sys.stderr)
        return 2
    scratch_disk=ROOT/'build/portdisk-m217.img'
    shutil.copyfile(ROOT/'portdisk.img',scratch_disk)
    marker=bytearray()
    for i in range(256):
        value=([0x4d56,0x5333,0x3753,0x5452][i] if i<4 else (0xa000|i))
        marker.extend((value&255,value>>8))
    with scratch_disk.open('r+b') as image:
        image.seek(8063*512)
        image.write(marker)
    if params_fixture:
        installer=ROOT/'build/install_params_fixture'
        result=subprocess.run([str(installer),str(scratch_disk)],check=False)
        if result.returncode:
            scratch_disk.unlink(missing_ok=True)
            return result.returncode
    machine=shutil.which('qemu-system-i386') or shutil.which('qemu-system-x86_64')
    if not machine:
        scratch_disk.unlink(missing_ok=True)
        print('QEMU M217: system QEMU not installed',file=sys.stderr)
        return 77
    cmd=[machine,'-display','none','-monitor','none','-serial','stdio',
         '-m','128','-boot','a','-no-reboot','-accel','tcg',
         '-drive','if=floppy,format=raw,file='+str(ROOT/'vms37lab.ima')+',snapshot=on',
         '-drive','if=ide,index=0,media=disk,format=raw,file='+str(scratch_disk)+',snapshot=on']
    p=subprocess.Popen(cmd,stdin=subprocess.DEVNULL,stdout=subprocess.PIPE,
                       stderr=subprocess.PIPE,bufsize=0)
    observed=bytearray()
    expected=[
        b'IA-32 VMS CPL1-to-CPL2 frame dispatch and interrupt return passed.',
        b'SYSBOOT: Files-11 SYS.EXE normalized 32-bit retrieval map stored.',
        b'SYSBOOT: Files-11 SYS.EXE native image loaded.',
        (b'SYSBOOT: Files-11 PARAMS.DAT applied before INIT allocation.'
         if params_fixture else
         b'SYSBOOT: PARAMS.DAT absent; source SYSPARAM defaults retained.'),
        b'INIT: loaded SYS.EXE initialized real IRP/LRP/SRP lookaside headers.',
        *([b'INIT: SYSBOOT PARAMS.DAT IRPCOUNT override reached resident IRP queue.']
          if params_fixture else []),
        b'SYSBOOT: Files-11 SYSLOA386.EXE native image loaded.',
        b'INIT: SYS.EXE linked live SYSLOA386 processor vectors.',
        b'SYSBOOT: Files-11 INILOA.EXE native image loaded.',
        b'INIT: INILOA IOMAP executed on real IA-32 boot adapter.',
        b'INIT: loaded SYS.EXE built lock ID, resource hash and deadlock bitmap tables.',
        b'INIT: SYS.EXE queued source resident NULL/SWAPPER PCBs; PHD/context activation pending.',
        b'INIT: source SCH$CHSE reached resident IA-32 vector 0x91; process contexts pending.',
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
        b'INIT: SYS.EXE FIL$READVBN verified real ATA SYS.EXE FH2 and native ELF32 via IRPs 8 and 9 (activation pending).',
        b'INIT: SYS.EXE FIL$READVBN verified complete native executable PT_LOAD text against resident SYS.EXE (activation pending).',
        b'INIT: SYS.EXE verified native SYSLOA386.EXE and INILOA.EXE text from real Files-11 ATA sectors (ACP unbound).',
        b'INIT: SYS.EXE FIL$READVBN verified unaligned 1024-byte range through three real ATA IRPs (ACP unbound).',
        b'INIT: SYS.EXE native FIL$FINDFILID reopened SYS.EXE through real MFD/SYSEXE/FH2 and verified 768 text bytes (ACP unbound).',
        b'INIT: SYS.EXE native FIL$FINDFILID enumerated real SYSEXE.DIR image FIDs via ATA (ACP unbound).',
        b'INIT: SYS.EXE FIL$RDCHKFILHDR collected source FH2 extent chain and verified 777 bytes via ATA (ACP unbound).',
        b'INIT: SYS.EXE FIL$READVBN cursor compared 901 resident bytes through four physical ATA IRPs (ACP unbound).',
        b'INIT: SYS.EXE source DQ_STARTIO private IOC queue completed three FIFO ATA IRPs with real resident text (DDT and ACP unbound).',
        b'INIT: SYS.EXE RETRYERR/IOCIOPOST posted three source FIFO ATA IOSBs after live resident reads (no DQA0 DDT or IRQ).',
        b'INIT: SYS.EXE FIL$READVBN used IOC private FIFO for two real ATA reads and matched 700 bytes (ACP/IRQ unbound).',
        b'INIT: SYS.EXE private owner-scoped channel assigned, read 600 live ATA bytes and deassigned (ACP/$QIO unbound).',
        b'INIT: SYS.EXE ATA sector reads completed through live primary IRQ14.',
        b'INIT: SYS.EXE FIN_DATABASE staged DQA0 names and resident SYS$DISK/SYS$SYSDEVICE (logical lookup pending).',
        b'INIT: SYS.EXE LOG$TRANSLATE resolved original SYS$DISK and SYS$SYSDEVICE names (system bootstrap only).',
        b'INIT: SYS.EXE executed console XON/XOFF/STOP/RESUME transitions (TT driver absent).',
        b'INIT: SYS.EXE IOC device search resolved NLA0 with native null DDT (others unbound).',
        b'INIT: SYS.EXE IOC$SEARCHDEV resolved SYS$DISK/SYS$SYSDEVICE to unbound DQA0 and NLA0 to linked DDT (no $ASSIGN).',
        b'INIT: SYS.EXE staged DQ DPTAB/DDTAB/FUNCTAB and native IRQ14 CRB; DQA0 DDT remains unbound.',
        b'INIT: SYS.EXE RELOCDRV IOC$INITDRV applied source DQ DPT_STORE to private DQA0 records (DDT/CRB unbound).',
        b'INIT: resident IOC$RELOC_DDT source DDT/FDT relocation verified (private only; DQA0 unbound).',
        b'INIT: SYS.EXE materialized native DQ DDT/FDT pointers (IOC publication pending).',
        b'INIT: SYS.EXE native DQ DDT STARTIO completed READPBLK through CRB IRQ14.',
        b'INIT: SYS.EXE allocated, completed and returned DQA0 IRP through resident source lookaside.',
        b'INIT: SYS.EXE NLDRIVER completed two IRPs via linked DDT/FDT and EXE$FINISHIOC (IPL4 requested).',
        b'INIT: SYS.EXE IOCIOPOST serviced two NLA IRPs through IA-32 CPL0 software INT 0x90 (no process AST).',
        b'INIT: loaded SYS.EXE resident entry executed and verified.',
        b'INIT: SWAPPER EXE$SWAPINIT built/searched SYS/GROUP RSHTs; SYS$DISK and SYS$SYSDEVICE resolve, process startup pending.',
        b'INIT: linked SYSLOA386 read real IDT, CR0, CR3, TSS, PIT and Files-11 boot home.',
        b'INIT: actual IRQ0 advanced resident SYS.EXE VMS clock.',
        b'INIT: SYS.EXE published native DQA0 DDT in DDB/UCB; IOC$SEARCHDEV found bound DQA0 (partial driver).',
        b'INIT: Files-11 FH2 chain read 700 SYS.EXE bytes across two sectors through IOC-selected DQA0 DDT/IRQ14.',
        b'INIT: ACP FCB source=3',
        b'INIT: ACP reclaimed rejected FCB allocation through MEMORYALC.',
        b'INIT: ACP read-only WCB MAKACC/DEACCS balanced FCB, VCB, and MEMORYALC state.',
        b'INIT: DQA0 UCB busy serialized two retained DDT READPBLK IRPs through IRQ14 and deferred IOC post.',
        b'INIT: DQA0 DDT asynchronous WRITEPBLK completed by IRQ14; scratch restored.',
        b'INIT: SYS.EXE DQA0 source-priority queue completed two ordered DDT WRITEPBLK IRPs; read queue posted two READPBLK IRPs via IRQ14, readback and restore verified.',
        b'INIT: bound source NULL/SWAPPER/SYSINIT PHDs to native IA-32 frames and process page tables.',
        b'SWAPPER: scheduled native context resolved SYS$DISK and SYS$SYSDEVICE.',
        b'SYSINIT: native scheduled context created and verified SYS$SYSTEM, SYS$SHARE and SYS$MESSAGE.',
        b'SYSINIT: SIP_SETTIME set the resident VMS clock from the IA-32 RTC.',
        b'SCH$WAITK blocked the active process; SCH$SCHED selected NULL.']
    failures=[
        b'*** x86 fault:',
        b'SYSBOOT: SYS.EXE retrieval map invalid.',
        b'SYSBOOT: PARAMS.DAT read or decode failed.',
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
        b'INIT: resident SYS.EXE physical ELF32 source probe failed.',
        b'INIT: resident native auxiliary disk text verification failed.',
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
        b'INIT: primary ATA IRQ14 did not complete a sector read.',
        b'INIT: live SYS.EXE PIT clock did not advance.',
        b'INIT: native NULL/SWAPPER PHD or context binding failed.',
        b'INIT: native process PHD/context binding failed at stage ',
        b'INIT: source SCH$CHSE reschedule request callback did not run.',
        b'INIT: source SCH$CHSE software reschedule interrupt was not delivered.',
        b'INIT: source SCH$CHSE did not reach native SCH$RESCHED.',
        b'SCH$WAITK scheduling failed with status ',
        b'SYSINIT: source SYSTEM logical creation failed.',
        b'SYSINIT: source logical translation verification failed.',
        b'SYSINIT: SIP_SETTIME could not read valid IA-32 RTC time.',
        b'INIT: marked QEMU ATA PIO write/readback/restore regression failed.',
        b'INIT: asynchronous ATA scratch write failed.',
        b'INIT: asynchronous ATA write readback failed.',
        b'INIT: asynchronous ATA write data mismatch.',
        b'INIT: asynchronous ATA scratch restore failed.',
        b'INIT: asynchronous ATA scratch restore mismatch.',
        b'INIT: DQA0 DDT native pointer/CRB publication validation failed.',
        b'INIT: IOC$SEARCHDEV did not find published DQA0.',
        b'INIT: IOC$SEARCHDEV did not select bound DQA0 for DQ I/O.',
        b'INIT: DQA0 DDB DDT START pointer validation failed.',
        b'INIT: IOC-selected DQA0 DDT STARTIO rejected DQ I/O.',
        b'INIT: DQA0 native DDT IOC publication or SEARCHDEV failed.',
        b'INIT: private asynchronous DQ IOC request setup failed.',
        b'INIT: asynchronous DQ IOC start/busy check failed.',
        b'INIT: async DQ IOC post failed.',
        b'INIT: published DQA0 async DDT/IOC/IRQ completion failed phase',
        b'INIT: IA-32 privilege transition stack is unprotected.',
        b'PORTLAB> ',b'Login successful',b'Login: ',b'\n$ ']
    passed=False
    success_deadline=None
    try:
        until=time.monotonic()+80
        while time.monotonic()<until:
            if any(x in observed for x in failures):break
            if all(x in observed for x in expected):
                positions=[observed.index(x) for x in expected]
                if positions!=sorted(positions):break
                if success_deadline is None:
                    # Keep the sole QEMU runner alive after NULL is selected
                    # so a bad IRET/process mapping or repeated SYSINIT wait
                    # can report a real guest fault before success.
                    success_deadline=time.monotonic()+3.0
                if time.monotonic()>=success_deadline:
                    passed=True
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
        serial_path=ROOT/'qemu-m217-serial.log'
        serial_path.write_bytes(observed)
        # Always retain the real transcript, including failed QEMU attempts.
        # GitHub run attempts get distinct keys; local runs get UTC keys.
        ci_run=os.environ.get('GITHUB_RUN_ID')
        ci_attempt=os.environ.get('GITHUB_RUN_ATTEMPT','1')
        identifier=(ci_run + ('-attempt'+ci_attempt if ci_attempt!='1' else '')
                    if ci_run else 'local-'+datetime.now(timezone.utc).strftime('%Y%m%dT%H%M%S%fZ'))
        run_url=('https://github.com/tuklusan/VAX-VMS-3.7-Baremetal-IA-32-Pentium/actions/runs/'+ci_run
                 if ci_run else '')
        capture(ROOT,serial_path,217,identifier,run_url,
                'harness-pass' if passed else 'harness-fail')
        scratch_disk.unlink(missing_ok=True)
    if passed:
        print('QEMU M217: 700-byte Files-11 chain read across two IOC-selected DQA0 DDT sectors via IRQ14 PASS')
        return 0
    print('QEMU M217: real guest execution or source-order check failed',file=sys.stderr)
    print(observed[-8192:].decode('ascii','replace'),file=sys.stderr)
    print(qemu_stderr.decode('ascii','replace'),file=sys.stderr)
    return 1
if __name__=='__main__':raise SystemExit(main())
