#!/usr/bin/env python3
"""Assert M109 IA-32 objects, source mapping, and unsupported scopes are honest."""
import json
import re
import subprocess
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
M=json.loads((ROOT/'tools/m109_originals.json').read_text())
assert M['revision']=='2f5bd967bc163bdb4aa9a5e4b1f9981d9995e752'
mods=M['modules']
assert M['file_count']==len(mods)==36
assert len({m['source'] for m in mods})==36
expected={entry['symbol'] for mod in mods for entry in mod['entries']}
assert len(expected)==64
for item in mods:
    file=ROOT/'src/native_rtl_m109'/(
        item['source'].split('/')[-1].lower().replace('.mar','.c'))
    assert file.is_file(),file
    code=file.read_text()
    assert 'No original restricted source is distributed' in code
    for ent in item['entries']:
        assert re.search(r'\b'+re.escape(ent['symbol'])+r'\s*\(',code)
    assert 0<item['entries'][0]['line']<item['lines']
    assert all(a['line']<b['line'] for a,b in zip(item['entries'],item['entries'][1:]))

partial=ROOT/'build/rtl-m109-partial.o'
raw=subprocess.check_output(['readelf','-h',str(partial)],text=True)
assert 'ELF32' in raw and 'Intel 80386' in raw and 'REL (Relocatable file)' in raw
undefined=subprocess.check_output(['nm','-u',str(partial)],text=True)
assert not undefined.strip(),undefined
names=subprocess.check_output(['nm','-g','--defined-only',str(partial)],text=True)
exports={line.rsplit(None,1)[-1] for line in names.splitlines() if line.strip()}
assert expected<=exports,sorted(expected-exports)

coverage=json.loads((ROOT/'source-coverage.json').read_text())
rows={row['original_file']:row for row in coverage['files']}
assert len(rows)==len(coverage['files'])
for m in mods:
    row=rows[m['source']]
    assert row['original_lines']==m['lines']
    sec=row['sections']
    assert sec[0]['original_lines'][0]==1
    assert sec[-1]['original_lines'][1]==m['lines']
    for i,s in enumerate(sec):
        assert s['status'] in ('partial','unresolved','excluded')
        a,b=s['original_lines']
        assert 1<=a<=b<=m['lines']
        if i: assert sec[i-1]['original_lines'][1]+1==a
    assert len([s for s in sec if s['status']=='partial'])==len(m['entries'])
print('M109: 36 distinct source files, 64 exports, ELF32 no undefined library dependencies, contiguous partial audit PASS')
