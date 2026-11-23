#!/usr/bin/env python3
"""Fail closed on untracked, repeated, or invented original-source line ranges.

This validator measures *inventory quality*, not executable parity. A partial
or unresolved original line never becomes a claim of full porting coverage.
"""
import argparse
import json
from collections import Counter
from pathlib import Path

ALLOWED = {'mapped', 'partial', 'unresolved', 'missing', 'excluded',
           'hardware-excluded'}
CODE = {'mapped', 'partial'}


def validate(document, root):
    errors = []
    seen = set()
    totals = Counter()
    for obj in document.get('files', []):
        name = obj.get('original_file')
        if not isinstance(name, str) or not name or name in seen:
            errors.append(f'{name}: duplicate or invalid source path')
            continue
        seen.add(name)
        n = obj.get('original_lines')
        if not isinstance(n, int) or n <= 0:
            errors.append(f'{name}: original line count missing')
            continue
        coverage = bytearray(n)
        if not obj.get('sections'):
            errors.append(f'{name}: source has no line-by-line sections')
            continue
        for index, section in enumerate(obj['sections']):
            label = f'{name} section {index}'
            pair = section.get('original_lines')
            if not isinstance(pair, list) or len(pair) != 2 or not all(isinstance(x,int) for x in pair):
                errors.append(f'{label}: missing line bounds')
                continue
            start, end = pair
            if not (1 <= start <= end <= n):
                errors.append(f'{label}: invalid original lines {pair} of {n}')
                continue
            status = section.get('status')
            if status not in ALLOWED:
                errors.append(f'{label}: unknown status {status!r}')
            totals[status] += end - start + 1
            for k in range(start-1, end):
                if coverage[k]:
                    errors.append(f'{name}: overlapping original line {k+1}')
                    break
                coverage[k] = 1
            if status in CODE:
                pcfile = section.get('pc_file')
                sym = section.get('pc_symbol')
                pcline = section.get('pc_line')
                if not isinstance(pcfile,str) or not isinstance(sym,str) or not sym:
                    errors.append(f'{label}: no native source/symbol')
                elif not (root/pcfile).is_file():
                    errors.append(f'{label}: native file absent: {pcfile}')
                if not isinstance(pcline,int) or pcline <= 0:
                    errors.append(f'{label}: native symbol line missing')
                elif pcfile and (root/pcfile).is_file() and pcline > sum(1 for _ in (root/pcfile).open()):
                    errors.append(f'{label}: native symbol line outside file')
            elif status in ('unresolved', 'missing', 'excluded', 'hardware-excluded'):
                if not section.get('reason'):
                    errors.append(f'{label}: no justification or unresolved dependency')
        gap = coverage.count(0)
        if gap:
            totals['unindexed_original_lines'] += gap
            pos = coverage.index(0) + 1
            errors.append(f'{name}: {gap} unmapped original lines (first line {pos})')
    if not seen:
        errors.append('No source file entries')
    return {'passed': not errors, 'indexed_original_files':len(seen),
            'reported_line_counts':sum(v.get('original_lines', 0) for v in document.get('files',[]) if isinstance(v.get('original_lines'),int)),
            'counts':dict(sorted(totals.items())), 'errors':errors}


def main():
    parser=argparse.ArgumentParser()
    parser.add_argument('--coverage',default='source-coverage.json')
    parser.add_argument('--root',default='.')
    parser.add_argument('--original-file-total',type=int,default=4566)
    parser.add_argument('--require-full-port',action='store_true')
    args=parser.parse_args()
    root=Path(args.root)
    result=validate(json.loads((root/args.coverage).read_text()),root)
    total=args.original_file_total
    result['original_repository_files']=total
    result['unindexed_original_files']=total-result['indexed_original_files']
    if total < result['indexed_original_files']:
        result['errors'].append('Indexed more files than pinned repository total')
    if args.require_full_port:
        if result['unindexed_original_files']:
            result['errors'].append('Full port incomplete: original files not indexed')
        for status in ('partial','unresolved','missing'):
            if result['counts'].get(status,0):
                result['errors'].append(f'Full port incomplete: {status} source lines remain')
    result['passed']=not result['errors']
    print(json.dumps(result,indent=2))
    return 0 if result['passed'] else 1


if __name__=='__main__':
    raise SystemExit(main())
