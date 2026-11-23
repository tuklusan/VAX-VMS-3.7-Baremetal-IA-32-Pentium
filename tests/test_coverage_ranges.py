#!/usr/bin/env python3
"""Structural audit quality test, including deliberately broken fixtures."""
import importlib.util
import json
from pathlib import Path

root=Path(__file__).resolve().parent.parent
spec=importlib.util.spec_from_file_location('validate_source_ranges',root/'tools'/'validate_source_ranges.py')
module=importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)
manifest=json.loads((root/'source-coverage.json').read_text())
report=module.validate(manifest,root)
assert report['passed'], report['errors'][:30]
assert report['counts'].get('unindexed_original_lines',0)==0
by_name={f['original_file']:f for f in manifest['files']}
for name in ('SYS/SRC/ASTDEL.MAR','SYS/SRC/POSTEF.MAR','SHRLIB/SYSDEF.MDL','SYS/SRC/INIT.MAR'):
    item=by_name[name]
    assert item['sections'] and item['original_lines']>0
    assert all('original_lines' in s for s in item['sections'])
assert any(s['status']=='unresolved' for s in by_name['SHRLIB/SYSDEF.MDL']['sections'])
assert len(manifest['files'])>=276

sample={'files':[{'original_file':'A.MAR','original_lines':4,'sections':[
    {'original_lines':[1,2],'status':'unresolved','reason':'source not ported'},
    {'original_lines':[3,4],'status':'excluded','reason':'comment'}]}]}
assert module.validate(sample,root)['passed']
for defect in ('gap','overlap','bad_status','missing_reason','missing_source'):
    broken=json.loads(json.dumps(sample))
    sections=broken['files'][0]['sections']
    if defect=='gap':sections[1]['original_lines']=[4,4]
    elif defect=='overlap':sections[1]['original_lines']=[2,4]
    elif defect=='bad_status':sections[0]['status']='done'
    elif defect=='missing_reason':del sections[0]['reason']
    elif defect=='missing_source':sections[0].update(status='mapped',pc_file='not-here.c',pc_symbol='test',pc_line=1)
    assert not module.validate(broken,root)['passed'], defect
print('Source line-range manifest overlap/gap/status regression PASS')
