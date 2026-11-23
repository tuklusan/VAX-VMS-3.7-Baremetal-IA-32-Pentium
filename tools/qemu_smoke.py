#!/usr/bin/env python3
"""Optional real-CPU-emulator smoke check (serial COM1, not static analysis).

Requires qemu-system-i386 or qemu-system-x86_64. Exits nonzero if not
installed, if no serial boot banner, if monitor commands fail, or if the
virtual machine exits unexpectedly. Captures a transcript for debugging.
"""
import argparse
import os
from pathlib import Path
import select
import shutil
import subprocess
import sys
import time

ROOT = Path(__file__).resolve().parent.parent

# Guest-level checks; exact success strings live in src/kernel.c.
CHECKS = (
    ("SHOW CLOCK", b"PIT IRQ0 ticks"),
    ("SHOW CPU", b"CPUID vendor"),
    ("SHOW SERIAL", b"COM1 active="),
    ("SHOW A20", b"A20 enabled and verified."),
    ("SHOW RAM", b"3MiB RAM window alias probe PASS."),
    ("TEST RING3", b"PASS: CPL3 entry, syscall, kernel stack, EXIT."),
    ("TEST DISK", b"PASS: native ATA PIO LBA1/LBA2, signature and checksum."),
    ("TEST USERFAULT", b"PASS: CPL3 write denied, #PF handled, EXIT."),
    ("TEST USERAST", b"PASS: queued AST delivered in ring3 via syscall."),
    ("TEST SCHED", b"PASS: two CPL3 contexts, cooperative switches and exit."),
    ("TEST PREEMPT", b"PASS: PIT preemption, two address spaces, AST IRET trampoline."),
    ("TEST PCB", b"PASS: PCB isolation, private allocated frames, reclaim."),
    ("TEST DEMAND", b"PASS: 4 demand faults, zeroed pages, CR3 isolation, reclaim."),
    ("SHOW VM", b"Demand faults="),
    ("SHOW MEMORY", b"Physical 4K frames free="),
    ("TEST KSTACK", b"PASS: per-PCB TSS.esp0, ring3 traps, guarded kernel stacks."),
    ("TEST EVENT", b"PASS: event AND block/wakeup, OR immediate, 2 CPL3 tasks."),
    ("TEST QIO", b"PASS: QIO 512-byte read, event flag and completion AST."),
    ("TEST USRDESC", b"PASS: CPL3 copy-in QIO descriptor and IDE completion."),
    ("TEST USRTEAR", b"PASS: process exit reclaimed owned QIO and channel."),
    ("TEST ASTDEL", b"PASS: ASTDEL process queue, KAST/IPAST, mode gating, swap/remove, wakeups."),
    ("TEST POSTEF", b"PASS: POSTEF PID/cluster set, AND/OR wake, common sync, backlog."),
    ("TEST MUTEX", b"PASS: MUTEX read/write, pending writers, wait queues, resources, IO mutex, priority."),
    ("TEST MUTEXCPL3", b"PASS: two CPL3 tasks, contested mutex, queued wake, RWAIT/RAVAIL, release."),
    ("TEST HPWD", b"PASS: HPWD CRC and Purdy login hashes, salt, username, match."),
    ("TEST VALIDATE", b"PASS: VALIDATE indexed UAF disk, search, passwords, DEFAULT, privileges."),
    ("TEST LOGINTIME", b"PASS: LIBDAYWK/LIBHOUR VMS epoch and live PC RTC."),
    ("TEST FILEIO", b"PASS: FILEIO real COM1 records, UAF password, process names and close."),
    ("TEST SYSTEMLOGIN", b"PASS: SYSTEM authenticated via original VMS UAF record and MANAGER CRC."),
    ("TEST DCLEXIT", b"PASS: DCL EXIT expression, indirect unstack, rundown, batch logout."),
    ("TEST DCLWAIT", b"PASS: DCL WAIT PIT timer, event 31, elapsed guest ticks."),
    ("TEST DCLINQUIRE", b"PASS: DCL INQUIRE prompt, options, symbol value, AST gating."),
)


# Repeated real-guest M49 stress checks; ordinary smoke remains 20 commands.
# After each demand run the allocator must have all 0x200 frames available.
STRESS_CHECKS = (
    ((('TEST DEMAND', b'PASS: 4 demand faults, zeroed pages, CR3 isolation, reclaim.'),
      ('SHOW MEMORY', b'Physical 4K frames free=0x00000200')) * 12)
    + (('TEST PREEMPT', b'PASS: PIT preemption, two address spaces, AST IRET trampoline.'),) * 4
    + (('TEST QIO', b'PASS: QIO 512-byte read, event flag and completion AST.'),) * 4
    + (('TEST USRTEAR', b'PASS: process exit reclaimed owned QIO and channel.'),) * 4
    + (('TEST EVENT', b'PASS: event AND block/wakeup, OR immediate, 2 CPL3 tasks.'),) * 4
    + (('TEST SCHED', b'PASS: two CPL3 contexts, cooperative switches and exit.'),) * 4
    + (('TEST ASTDEL', b'PASS: ASTDEL process queue, KAST/IPAST, mode gating, swap/remove, wakeups.'),) * 12
    + (('TEST POSTEF', b'PASS: POSTEF PID/cluster set, AND/OR wake, common sync, backlog.'),) * 12
    + (('TEST MUTEX', b'PASS: MUTEX read/write, pending writers, wait queues, resources, IO mutex, priority.'),) * 4
    + (('TEST MUTEXCPL3', b'PASS: two CPL3 tasks, contested mutex, queued wake, RWAIT/RAVAIL, release.'),) * 4
)


def run_smoke(binary: str, boot: Path, disk: Path, timeout: float, stress: bool=False) -> int:
    cmd = [
        binary, '-display', 'none', '-monitor', 'none', '-serial', 'stdio',
        '-m', '128', '-boot', 'a', '-no-reboot', '-accel', 'tcg',
        '-drive', f'if=floppy,format=raw,file={boot},snapshot=on',
        '-drive', f'if=ide,index=0,media=disk,format=raw,file={disk},snapshot=on',
    ]
    proc = subprocess.Popen(cmd, stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                            stderr=subprocess.PIPE, bufsize=0)
    log = bytearray()
    start = time.monotonic()
    transcript = ROOT/'qemu-smoke.log'

    def until(match: bytes, seconds: float, from_offset: int = 0) -> bool:
        end = min(time.monotonic()+seconds, start+timeout)
        while time.monotonic() < end:
            if match in log[from_offset:]:
                return True
            if proc.poll() is not None:
                return False
            remaining = max(0.0, min(0.5, end-time.monotonic()))
            ready, _, _ = select.select([proc.stdout], [], [], remaining)
            if ready:
                chunk = os.read(proc.stdout.fileno(), 4096)
                if not chunk:
                    return False
                log.extend(chunk)
        return match in log[from_offset:]

    try:
        if not until(b'PORTLAB> ', 20.0):
            raise RuntimeError('boot banner/prompt absent on COM1')
        checks = CHECKS + (STRESS_CHECKS if stress else ())
        for command_text, expected in checks:
            command = (command_text + '\r').encode('ascii')
            start_pos = len(log)
            for char in command:
                proc.stdin.write(bytes([char]))
                proc.stdin.flush()
                time.sleep(0.02)
            if command_text == "TEST FILEIO":
                for prompt, answer in ((b"FILEIO INPUT> ", b"labuser\r"),
                                       (b"FILEIO PASSWORD> ", b"password\r")):
                    if not until(prompt, 15.0, start_pos):
                        raise RuntimeError('FILEIO guest prompt absent: %r' % prompt)
                    for char in answer:
                        proc.stdin.write(bytes([char]))
                        proc.stdin.flush()
                        time.sleep(0.02)
            if command_text == "TEST SYSTEMLOGIN":
                for prompt, answer in ((b"Username: ", b"SYSTEM\r"),
                                       (b"Password: ", b"MANAGER\r")):
                    if not until(prompt, 15.0, start_pos):
                        raise RuntimeError('SYSTEM guest prompt absent: %r' % prompt)
                    for char in answer:
                        proc.stdin.write(bytes([char]))
                        proc.stdin.flush()
                        time.sleep(0.02)
            if command_text == "TEST INTERACT":
                for prompt, answer in ((b"Username: ", b"labuser\r"),
                                       (b"Password: ", b"password\r")):
                    if not until(prompt, 15.0, start_pos):
                        raise RuntimeError('INTERACT guest prompt absent: %r' % prompt)
                    for char in answer:
                        proc.stdin.write(bytes([char]))
                        proc.stdin.flush()
                        time.sleep(0.02)
            # Look only at output following this command, not prior commands.
            while expected not in log[start_pos:]:
                if not until(expected, 20.0, start_pos):
                    raise RuntimeError(f'{command!r} did not answer with {expected!r}')
            while b'PORTLAB> ' not in log[start_pos:]:
                if not until(b'PORTLAB> ', 15.0, start_pos):
                    raise RuntimeError(f'{command!r} did not return to prompt')
        print(f'QEMU COM1 guest boot + {len(checks)} monitor commands: PASS')
        return 0
    except RuntimeError as err:
        print(f'QEMU COM1 guest smoke: FAIL: {err}', file=sys.stderr)
        return 1
    finally:
        try:
            proc.terminate()
            proc.wait(timeout=3)
        except subprocess.TimeoutExpired:
            proc.kill()
            proc.wait()
        log.extend(b'\n=== QEMU stderr ===\n')
        if proc.stderr:
            log.extend(proc.stderr.read(8192))
        transcript.write_bytes(log)
        print(f'Guest transcript: {transcript}')


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument('--emulator', default=None)
    parser.add_argument('--timeout', type=float, default=240.0)
    parser.add_argument('--stress', action='store_true')
    opts = parser.parse_args()
    exe = (opts.emulator or shutil.which('qemu-system-i386')
           or shutil.which('qemu-system-x86_64'))
    if not exe:
        print('QEMU guest smoke: NOT RUN (QEMU executable not installed)', file=sys.stderr)
        return 77
    return run_smoke(exe, ROOT/'vms37lab.ima', ROOT/'portdisk.img', opts.timeout, opts.stress)


if __name__ == '__main__':
    sys.exit(main())
