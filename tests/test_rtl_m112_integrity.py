#!/usr/bin/env python3
"""M112 original-specific source, IA-32 ELF32 symbols and partial-ranges audit."""
import json,re,subprocess
from pathlib import Path
r=Path(__file__).resolve().parents[1]
m=json.loads((r/'tools/m112_originals.json').read_text())
assert m['file_count']==40 and m['dispatch_files']==30 and m['core_files']==10
assert m['revision']=='2f5bd967bc163bdb4aa9a5e4b1f9981d9995e752'
files={x['original_file']:x for x in json.loads((r/'source-coverage.json').read_text())['files']}
assert len(files)>=390,(len(files),'expected new source indexed')
seen=set();syms=set();npartial=nunresolved=0
for mod in m['modules']:
 assert mod['source'] not in seen;seen.add(mod['source'])
 p=r/'src/native_rtl_m112'/(mod['name'].lower()+'.c')
 assert p.is_file()
 s=p.read_text()
 native=set(re.findall(r'\bint\s+(port112_[A-Za-z_0-9]+)\s*\(',s))
 for e in mod['entries']:
  assert e['symbol'] in native,e
  syms.add(e['symbol'])
 entry=files[mod['source']]
 assert entry['original_lines']==mod['lines']
 expected=1
 for a in entry['sections']:
  assert a['original_lines'][0]==expected,(mod['source'],a,expected)
  expected=a['original_lines'][1]+1
  if a['status']=='partial':
   npartial+=1
   assert a['pc_symbol'] in native
   assert a['pc_line']>=1
  elif a['status']=='unresolved':
   nunresolved+=1
   assert a['reason']
  else:
   assert a['status']=='excluded'
 assert expected==mod['lines']+1
assert len(seen)==40 and len(syms)==42
assert npartial==42 and nunresolved==40
obj=r/'build/rtl-m112-partial.o'
h=subprocess.check_output(['readelf','-h',str(obj)],text=True)
assert 'ELF32' in h and 'Intel 80386' in h
assert not subprocess.check_output(['nm','-u',str(obj)],text=True).strip()
pub={x.split()[-1] for x in subprocess.check_output(['nm','-g','--defined-only',str(obj)],text=True).splitlines() if x}
assert syms<=pub,(syms-pub)
print('M112: 40 distinct pinned FORTRAN source files, 42 ELF32 entry symbols, 42 partial + 40 unresolved ranges PASS')
