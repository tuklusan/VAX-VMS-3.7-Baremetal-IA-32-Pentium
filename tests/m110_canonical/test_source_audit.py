import json
import tempfile
from pathlib import Path
from subprocess import run

script = Path(__file__).resolve().parent.parent / 'tools/source_audit.py'
with tempfile.TemporaryDirectory() as temp:
    root = Path(temp) / 'tree'
    root.mkdir()
    (root/'SYS'/'SRC').mkdir(parents=True)
    (root/'SYS'/'SRC'/'ASTDEL.MAR').write_text('test fixture', encoding='ascii')
    check = run(['python3', str(script), str(root)], text=True, capture_output=True, check=True)
    doc = json.loads(check.stdout)
    assert doc['total_files'] == 1
    assert doc['all_checklist_paths_present'] is False
    row = doc['areas']['asynchronous_traps'][0]
    assert row['present'] is True and len(row['sha256']) == 64
    assert doc['areas']['boot'][0]['present'] is False
print('Source inventory fixture checks PASS')
