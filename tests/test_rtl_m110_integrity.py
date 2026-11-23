#!/usr/bin/env python3
"""Check all 34 new source files, real ELF32 symbols and partial line ranges."""
import json
import re
import subprocess
from pathlib import Path
root=Path(__file__).resolve().parents[1]
mods=json.loads((root/'tools/m110_originals.json').read_text())
assert mods['revision']=='2f5bd967bc163bdb4aa9a5e4b1f9981d9995e752'
assert mods['file_count']==34 and len(mods['modules'])==34
assert len({x['source'] for x in mods['modules']})==34
coverage=json.loads((root/'source-coverage.json').read_text())
rows={x['original_file']:x for x in coverage['files']}
assert len(rows)==len(coverage['files']) and len(rows)>=310  # later milestones may add sources
symbols=set()
partial_count=0
for mod in mods['modules']:
    name=mod['source'].split('/')[-1].lower().replace('.mar','.c')
    path=root/'src/native_rtl_m110'/name
    assert path.is_file()
    source=path.read_text()
    assert 'DEC source' in source or 'DEC text' in source or 'restricted source' in source or 'source copied' in source
    exported=re.findall(r'\bint\s+(port110_[a-zA-Z0-9_]+)\s*\(',source)
    assert exported,(mod['source'],'no native callable entry')
    assert len(set(exported))==len(exported)
    symbols.update(exported)
    row=rows[mod['source']]
    assert row['original_lines']==mod['lines']
    sections=row['sections']
    assert sections[0]['original_lines'][0]==1 and sections[-1]['original_lines'][1]==mod['lines']
    for idx,item in enumerate(sections):
        assert item['status'] in ('partial','unresolved','excluded')
        if idx: assert sections[idx-1]['original_lines'][1]+1==item['original_lines'][0]
        if item['status']=='partial':
            partial_count+=1
            assert item['pc_symbol'] in exported
            assert item['pc_line']>=1
        elif item['status']=='unresolved':
            assert item.get('reason')
    assert any(item['status']=='unresolved' for item in sections),mod['source']

obj=root/'build/rtl-m110-partial.o'
elf=subprocess.check_output(['readelf','-h',str(obj)],text=True)
assert 'ELF32' in elf and 'Intel 80386' in elf and 'REL (Relocatable file)' in elf
undefined=subprocess.check_output(['nm','-u',str(obj)],text=True)
assert not undefined.strip(),undefined
visible=subprocess.check_output(['nm','-g','--defined-only',str(obj)],text=True)
exports={line.rsplit(None,1)[-1] for line in visible.splitlines() if line.strip()}
assert symbols<=exports,sorted(symbols-exports)
assert len(symbols)>=60
assert partial_count>=40
for s in ('SYS/SRC/ASTDEL.MAR','SYS/SRC/POSTEF.MAR'):
    assert all(x['status']!='mapped' for x in rows[s]['sections'])
print(f'M110: 34 RTL source wrappers, {len(symbols)} native ELF32 symbols, '
      f'{partial_count} partial entry ranges and two SYS audits PASS')
