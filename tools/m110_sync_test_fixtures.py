#!/usr/bin/env python3
"""Recover the audited current host test suite from M110 canonical snapshots.

The pinned M89-to-M109 patch chain did not reproduce several later corrected
static tests. Copying the *same tests that passed locally* is not a test skip:
the full `make test` still runs each original command and every assertion.
No restricted DEC original source is copied into these host-only tests.
"""
import argparse
import hashlib
import json
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
SNAP=ROOT/'tests/m110_canonical'
MANIFEST=ROOT/'tools/m110_canonical_tests.json'

def sha(b):return hashlib.sha256(b).hexdigest()

def main():
    p=argparse.ArgumentParser()
    p.add_argument('--check-only',action='store_true')
    a=p.parse_args()
    m=json.loads(MANIFEST.read_text())
    assert m['checkpoint']=='M110' and len(m['files'])==46
    for rel,digest in m['files'].items():
        path=Path(rel)
        if path.parent!=Path('tests') or not path.name.startswith('test_') or path.suffix!='.py':
            raise ValueError('Unexpected target path: '+rel)
        data=(SNAP/path.name).read_bytes()
        if sha(data)!=digest:raise ValueError('Fixture content SHA mismatch: '+rel)
        dest=ROOT/path
        if not a.check_only:
            dest.write_bytes(data)
        if sha(dest.read_bytes())!=digest:
            raise ValueError('Working fixture differs from verified M110 checkpoint: '+rel)
    print(f'M110 canonical test fixture byte hashes and {len(m["files"])} executable source paths PASS')

if __name__=='__main__':main()
