from pathlib import Path
import subprocess
r=Path(__file__).resolve().parent.parent
k=(r/'src/kernel.c').read_text()
for t in ('TEST SESSION','PORT_SESSION_MIRROR_OBJECT','port_session_build_request',
          'port_session_parse_request'):
    assert t in k,t
nm=subprocess.check_output(['nm',str(r/'build/kernel.elf')],text=True)
for t in ('port_session_build_request','port_session_parse_request'):
    assert t in nm,t
print('M18 Session Control diagnostic linked/guest-integrated PASS')
