#!/usr/bin/env python3
"""M111 40-module source-specific ELF32, pinned line ranges and no false coverage."""
import json
import re
import subprocess
from pathlib import Path
root=Path(__file__).resolve().parents[1]
manifest=json.loads((root/'tools/m111_originals.json').read_text())
assert manifest['revision']=='2f5bd967bc163bdb4aa9a5e4b1f9981d9995e752'
assert manifest['file_count']==40 and len(manifest['modules'])==40
assert len({x['source'] for x in manifest['modules']})==40
inventory=json.loads((root/'source-coverage.json').read_text())
rows={x['original_file']:x for x in inventory['files']}
assert len(rows)==len(inventory['files']) and len(rows)>=350
syms=set();partial=0;unresolved=0
for mod in manifest['modules']:
    path=root/'src/native_rtl_m111'/(mod['name'].lower()+'.c')
    assert path.is_file(),str(path)
    source=path.read_text()
    assert 'Copyrighted DEC source text is NOT copied' in source
    exported=re.findall(r'\bint\s+(port111_[a-zA-Z0-9_]+)\s*\(',source)
    assert exported,(path,'no native callable entry')
    assert len(set(exported))==len(exported)
    syms.update(exported)
    row=rows[mod['source']]
    assert row['original_lines']==mod['lines']
    secs=row['sections']
    assert secs[0]['original_lines'][0]==1 and secs[-1]['original_lines'][1]==mod['lines']
    cursor=1
    for sec in secs:
        assert sec['original_lines'][0]==cursor,(mod['source'],sec,cursor)
        cursor=sec['original_lines'][1]+1
        assert sec['status'] in ('partial','unresolved','excluded'),sec
        if sec['status']=='partial':
            partial+=1
            assert sec['pc_file']=='src/native_rtl_m111/'+mod['name'].lower()+'.c'
            assert sec['pc_symbol'] in exported
            assert sec['pc_line']>0
        if sec['status']=='unresolved':
            unresolved+=1
            assert 'reason' in sec
    assert cursor==mod['lines']+1
    assert secs[0]['status']=='unresolved'
assert partial==51,(partial,'expected mapped entry count')
assert unresolved==40,(unresolved,'expected explicit unresolved source preambles')
obj=root/'build/rtl-m111-partial.o'
elf=subprocess.check_output(['readelf','-h',str(obj)],text=True)
assert 'ELF32' in elf and 'Intel 80386' in elf and 'REL (Relocatable file)' in elf
missing=subprocess.check_output(['nm','-u',str(obj)],text=True)
assert not missing.strip(),missing
names=subprocess.check_output(['nm','-g','--defined-only',str(obj)],text=True)
exports={s.split()[-1] for s in names.splitlines() if s.strip()}
assert syms<=exports,sorted(syms-exports)
assert len(syms)==51,(len(syms),sorted(syms))
print('M111: 40 distinct pinned original source files, 51 source-specific ELF32 symbols; 51 partial / 40 unresolved ranges PASS')
