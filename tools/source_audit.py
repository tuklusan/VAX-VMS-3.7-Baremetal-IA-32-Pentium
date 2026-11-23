#!/usr/bin/env python3
"""Read-only inventory of a locally obtained VMS 3.7 source tree.

Does not download, alter, reproduce, compile, or redistribute DEC source.
Input is a local checkout provided by the operator with appropriate rights.
"""
from __future__ import annotations
import argparse
import hashlib
import json
from collections import Counter
from pathlib import Path

# Source paths checked against the public repository's directory indices.
AREAS = {
    'boot': ['BOOTS/SRC/VMB.MAR', 'BOOTS/SRC/SYSBOOT.MAR', 'BOOTS/SRC/LOADER.MAR'],
    'kernel_early': ['SYS/SRC/INIT.MAR', 'SYS/SRC/INITVEC.MAR', 'SYS/SRC/SCBVECTOR.MAR'],
    'exceptions': ['SYS/SRC/EXCEPTION.MAR', 'SYS/SRC/PAGEFAULT.MAR'],
    'scheduler': ['SYS/SRC/SCHED.MAR', 'SYS/SRC/PROCSTRT.MAR', 'SYS/SRC/SWAPPER.MAR'],
    'asynchronous_traps': ['SYS/SRC/ASTDEL.MAR', 'SYS/SRC/IPCONTROL.MAR'],
    'disk_and_io': ['SYS/SRC/IOPERFORM.MAR', 'DRIVER/SRC/DQDRIVER.MAR'],
    'dcl': ['DCL/SRC/COMMAND.MAR', 'DCL/SRC/DCLPARSE.MAR'],
    'rms': ['RMS/SRC/RM1GET.MAR', 'RMS/SRC/RM1PUT.MAR'],
    'decnet': ['NETACP/SRC/NSP.MAR', 'NETACP/SRC/NSPXPORT.MAR',
               'NCP/SRC/NCPMAIN.B32', 'FAL/SRC/FALMAIN.MAR'],
    'source_build': ['SYSBLDCOM/SOURCEKIT.RNO', 'SYSBLDCOM/VMSNEWBLD.COM'],
}

def audit(root: Path) -> dict:
    root = root.resolve(strict=True)
    if not root.is_dir():
        raise ValueError('input must be a directory')
    counts = Counter()
    for p in root.rglob('*'):
        if p.is_file() and '.git' not in p.relative_to(root).parts:
            counts[p.suffix.upper() or '(NONE)'] += 1
    areas = {}
    for area, paths in AREAS.items():
        entries = []
        for rel in paths:
            p = root / rel
            if p.is_file() and p.resolve().is_relative_to(root):
                blob = p.read_bytes()
                entries.append({'path': rel, 'present': True,
                                'size_bytes': len(blob),
                                'sha256': hashlib.sha256(blob).hexdigest()})
            else:
                entries.append({'path': rel, 'present': False})
        areas[area] = entries
    return {
        'input': str(root),
        'notice': 'Inventory only; source-file copyright and licensing unchanged',
        'total_files': sum(counts.values()),
        'extensions': dict(sorted(counts.items())),
        'areas': areas,
        'all_checklist_paths_present': all(e['present'] for v in areas.values() for e in v),
    }

def main() -> None:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('source_tree', type=Path)
    ap.add_argument('--output', type=Path, help='Save a JSON inventory (no source text)')
    args = ap.parse_args()
    report = audit(args.source_tree)
    body = json.dumps(report, indent=2, sort_keys=True) + '\n'
    if args.output:
        args.output.write_text(body, encoding='utf-8')
        print('Wrote metadata inventory:', args.output)
    else:
        print(body)

if __name__ == '__main__':
    main()
