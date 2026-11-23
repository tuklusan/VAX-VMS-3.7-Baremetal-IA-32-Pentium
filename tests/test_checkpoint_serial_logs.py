#!/usr/bin/env python3
"""Regression tests: raw serial logs survive bundle and cannot be overwritten."""
from pathlib import Path
import json
import subprocess
import sys
import tempfile
from zipfile import ZipFile

SCRIPT = Path(__file__).resolve().parents[1] / 'tools/checkpoint_serial_logs.py'

def invoke(*args, ok=True):
    p = subprocess.run([sys.executable, str(SCRIPT), *map(str, args)],
                       capture_output=True, text=True)
    assert (p.returncode == 0) == ok, (p.stdout, p.stderr)
    return p

def main():
    with tempfile.TemporaryDirectory() as d:
        root = Path(d) / 'native-m93'
        root.mkdir()
        for x in ('src', 'tools', 'tests', 'build'):
            (root / x).mkdir()
            (root / x / 'keep.txt').write_text(x)
        for x in ('Makefile','source-coverage.json','resume.md','vms37lab.ima','portdisk.img'):
            (root / x).write_text(x)
        log = Path(d) / 'from-qemu.log'
        raw = b'COM1\r\r\nCPU initialized\r\n=== QEMU stderr ===\n'
        log.write_bytes(raw)
        params = ('capture','--root',root,'--source',log,'--milestone','93',
                  '--run-id','12345','--run-url','https://github.com/example/actions/runs/12345',
                  '--result','pass')
        invoke(*params)
        invoke(*params)  # Same bytes are safe/idempotent.
        invoke('verify','--root',root)
        log.write_bytes(b'DIFFERENT')
        invoke(*params, ok=False) # Never overwrite old run content.
        log.write_bytes(raw)
        bundle = Path(d) / 'new.zip'
        invoke('bundle','--root',root,'--output',bundle)
        with ZipFile(bundle) as z:
            assert z.read('native-m93/logs/serial/qemu-m93-run-12345.log') == raw
            assert z.read('native-m93/src/keep.txt') == b'src'
            assert z.read('native-m93/tools/keep.txt') == b'tools'
            assert z.read('native-m93/portdisk.img') == b'portdisk.img'
        p = root / 'logs/serial/qemu-m93-run-12345.log'
        p.write_bytes(b'CORRUPTED')
        invoke('verify','--root',root,ok=False)
        invoke('bundle','--root',root,'--output',bundle,ok=False)
    print('QEMU COM1 evidence capture, anti-overwrite, integrity and full checkpoint ZIP PASS')

if __name__ == '__main__':
    main()
