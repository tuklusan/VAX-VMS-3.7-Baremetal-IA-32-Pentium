#!/usr/bin/env python3
"""M113 original-source provenance, contiguous partial audit and freestanding IA-32."""
import json
from pathlib import Path
import subprocess
ROOT=Path(__file__).resolve().parents[1]
manifest=json.loads((ROOT/'tools/m113_originals.json').read_text())
coverage=json.loads((ROOT/'source-coverage.json').read_text())
assert manifest['revision']=='2f5bd967bc163bdb4aa9a5e4b1f9981d9995e752'
assert manifest['file_count']==len(manifest['modules'])==40
files={x['original_file']:x for x in coverage['files']}
assert len(files)==len(coverage['files']) and 430<=len(files)<=4566,(len(files),len(coverage['files']))
assert len(list((ROOT/'src/native_rtl_m113').glob('*.c')))==40
syms=[]
for mod in manifest['modules']:
    name=mod['name'];source=mod['source'];original=files[source]
    native=ROOT/'src/native_rtl_m113'/f'{name.lower()}.c'
    assert native.is_file()
    assert original['original_lines']==mod['lines']
    ranges=original['sections']
    assert ranges[0]['original_lines'][0]==1
    assert ranges[-1]['original_lines'][1]==mod['lines']
    assert ranges[-1]['status']=='excluded'
    assert all(a['original_lines'][1]+1==b['original_lines'][0] for a,b in zip(ranges,ranges[1:])),name
    for e in mod['entries']:
        symbol=e['symbol'];syms.append(symbol)
        assert symbol in native.read_text()
        sections=[s for s in ranges if s.get('pc_symbol')==symbol]
        assert len(sections)==1 and sections[0]['status']==e['status'],(name,symbol)
        assert sections[0]['pc_line'] and sections[0]['pc_line']>0
assert len(syms)==len(set(syms))==43
out=subprocess.check_output(['nm','-g','--defined-only',str(ROOT/'build/rtl-m113-partial.o')],text=True)
for symbol in syms:
    assert (' '+symbol+'\n') in out,symbol
u=subprocess.check_output(['nm','-u',str(ROOT/'build/rtl-m113-partial.o')],text=True)
assert not any(t in u for t in ('__divdi3','__udivdi3','__moddi3','__umoddi3','memcpy','strlen','malloc','printf')),u
hdr=subprocess.check_output(['readelf','-h',str(ROOT/'build/rtl-m113-partial.o')],text=True)
assert 'ELF32' in hdr and '80386' in hdr
assert sum(len(v['sections']) for v in coverage['files']) >= 1800
print('M113: 40 pinned source files, 43 source-specific ELF32 symbols, explicitly PARTIAL/unresolved contiguous audit PASS')
