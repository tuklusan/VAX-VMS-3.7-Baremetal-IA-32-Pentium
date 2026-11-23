#!/usr/bin/env python3
"""Keep raw, cumulative QEMU COM1 logs inside every recoverable source ZIP.

This tool never edits serial bytes.  No log is called a successful boot merely
because it appears in the manifest.  `bundle` carries the full source/build
working tree, including its disk images, tests and scripts.
"""
import argparse
from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path
import re
import sys
from zipfile import ZipFile, ZIP_DEFLATED

STORE = Path('logs/serial')
MANIFEST = STORE / 'manifest.json'
REQUIRED = ('Makefile', 'source-coverage.json', 'resume.md',
            'vms37lab.ima', 'portdisk.img')
NAME = re.compile(r'^qemu-m([0-9]+)-run-([0-9]+(?:-attempt[0-9]+)?|local-[0-9TZ]+)\.log$')

def sha(data):
    return hashlib.sha256(data).hexdigest()

def load(root):
    path = root / MANIFEST
    if not path.is_file():
        return {'schema_version': 1, 'kind': 'raw-qemu-serial-archive', 'runs': []}
    m = json.loads(path.read_text(encoding='utf-8'))
    if m.get('schema_version') != 1 or m.get('kind') != 'raw-qemu-serial-archive' or not isinstance(m.get('runs'), list):
        raise ValueError('Unrecognized serial manifest')
    return m

def capture(root, source, milestone, run_id, run_url, result):
    if not source.is_file():
        raise ValueError(f'Missing real QEMU transcript: {source}')
    data = source.read_bytes()
    if not data:
        raise ValueError('Refusing to archive an empty QEMU transcript')
    milestone = str(milestone).lower().removeprefix('m')
    run_id = str(run_id)
    if not milestone.isdecimal() or not (re.fullmatch(r'[0-9]+(?:-attempt[0-9]+)?|local-[0-9TZ]+', run_id)):
        raise ValueError('Invalid milestone or run identifier')
    relative = STORE / f'qemu-m{milestone}-run-{run_id}.log'
    target = root / relative
    existing = load(root)
    if target.exists() and target.read_bytes() != data:
        raise ValueError(f'Log collision (would overwrite history): {relative}')
    if not target.exists():
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_bytes(data)
    record = {
        'milestone': f'M{milestone}', 'run_id': run_id,
        'run_url': run_url, 'test_result': result,
        'path': relative.as_posix(), 'bytes': len(data), 'sha256': sha(data)
    }
    for old in existing['runs']:
        if old['path'] == record['path']:
            if old != record:
                raise ValueError(f'Conflicting manifest entry: {relative}')
            return
    existing['runs'].append(record)
    existing['runs'].sort(key=lambda r: (int(r['milestone'][1:]), r['run_id']))
    (root / MANIFEST).write_text(json.dumps(existing, indent=2) + '\n', encoding='utf-8')

def verify(root):
    m = load(root)
    if not m['runs']:
        raise ValueError('Serial history is empty')
    known = set()
    for run in m['runs']:
        rel = run['path']
        p = Path(rel)
        if p.is_absolute() or p.parent != STORE or not NAME.fullmatch(p.name):
            raise ValueError(f'Unsafe serial log path: {rel}')
        if rel in known:
            raise ValueError(f'Duplicate serial history entry: {rel}')
        known.add(rel)
        data = (root / p).read_bytes()
        if len(data) != run['bytes'] or sha(data) != run['sha256']:
            raise ValueError(f'Corrupt or changed raw QEMU serial transcript: {rel}')
        if run['milestone'].lower() != 'm' + NAME.fullmatch(p.name).group(1):
            raise ValueError(f'Milestone does not match {rel}')
        if run['run_id'] != NAME.fullmatch(p.name).group(2):
            raise ValueError(f'Run ID does not match {rel}')
    actual = {p.relative_to(root).as_posix() for p in (root / STORE).glob('*.log')}
    if actual != known:
        raise ValueError(f'Untracked or absent serial log(s): {actual ^ known}')
    return m

def bundle(root, destination):
    m = verify(root)
    missing = [x for x in REQUIRED if not (root / x).is_file()]
    missing += [x for x in ('src', 'tools', 'tests', 'build') if not (root / x).is_dir()]
    if missing:
        raise ValueError('Source/build checkpoint incomplete: ' + ', '.join(missing))
    destination = destination.resolve()
    if root == destination or destination.is_relative_to(root):
        raise ValueError('ZIP must be written outside the tree being archived')
    files = sorted(p for p in root.rglob('*') if p.is_file() and '.git' not in p.parts and '__pycache__' not in p.parts)
    if not files or any(p.is_symlink() for p in files):
        raise ValueError('Missing files or unsupported symbolic links')
    destination.parent.mkdir(parents=True, exist_ok=True)
    with ZipFile(destination, 'w', compression=ZIP_DEFLATED, compresslevel=6) as z:
        for file in files:
            z.write(file, f'{root.name}/{file.relative_to(root).as_posix()}')
    with ZipFile(destination) as z:
        if z.testzip():
            raise ValueError('ZIP CRC verification failed')
        if len(z.namelist()) != len(files):
            raise ValueError('ZIP file count mismatch')
        for x in REQUIRED:
            if f'{root.name}/{x}' not in z.namelist():
                raise ValueError('Missing checkpoint file in ZIP: ' + x)
        for run in m['runs']:
            content = z.read(f'{root.name}/{run["path"]}')
            if len(content) != run['bytes'] or sha(content) != run['sha256']:
                raise ValueError('Serial evidence corrupt inside ZIP: ' + run['path'])
    print(f'Validated complete source/build ZIP: {destination}, {len(files)} files, {len(m["runs"])} QEMU logs')

def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('command', choices=('capture', 'verify', 'bundle'))
    ap.add_argument('--root', type=Path, default=Path(__file__).resolve().parents[1])
    ap.add_argument('--source', type=Path)
    ap.add_argument('--milestone')
    ap.add_argument('--run-id')
    ap.add_argument('--run-url', default='')
    ap.add_argument('--result', default='not-assessed')
    ap.add_argument('--output', type=Path)
    args = ap.parse_args()
    root = args.root.resolve()
    if args.command == 'capture':
        if not args.source or not args.milestone:
            ap.error('capture requires --source and --milestone')
        stamp = 'local-' + datetime.now(timezone.utc).strftime('%Y%m%dT%H%M%SZ')
        capture(root, args.source, args.milestone, args.run_id or stamp,
                args.run_url, args.result)
        verify(root)
    elif args.command == 'verify':
        print(f'QEMU serial manifest verified: {len(verify(root)["runs"])} raw logs')
    else:
        if not args.output:
            ap.error('bundle requires --output')
        bundle(root, args.output)

if __name__ == '__main__':
    try:
        main()
    except (OSError, ValueError, KeyError, json.JSONDecodeError) as exc:
        print('Checkpoint serial log error: ' + str(exc), file=sys.stderr)
        raise SystemExit(1)
