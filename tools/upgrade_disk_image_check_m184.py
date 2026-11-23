#!/usr/bin/env python3
"""Upgrade M110's frozen format-1-only image check for an ODS-2 format-2 SYS.EXE.

The upstream M89 clean-rebuild recipe and archived M176 tree can carry
slightly different copies of this historic test. Canonicalize it first,
then extend only the retrieval-pointer decoding. The test still verifies
real on-disk FID, checksum, extent bounds, no overlaps and IA-32 ELF.
"""
from pathlib import Path

root=Path(__file__).resolve().parents[1]
source=root/'tests/m110_canonical/test_disk_image.py'
target=root/'tests/test_disk_image.py'
s=source.read_text()
old='''    assert b[58]==2 # one format-1 extent, as installed by native image tools
    first=word(b,132)
    blocks=1+(word(b,130)&0xff)
'''
new='''    # A growing IA-32 SYS.EXE can exceed 256 blocks: decode native ODS-2
    # FH2 retrieval-pointer formats 1 and 2, not a hardcoded file size.
    if b[58]==2:
        assert (word(b,130)>>14)==1
        first=word(b,132)|(((word(b,130)>>8)&0x3f)<<16)
        blocks=1+(word(b,130)&0xff)
    else:
        assert b[58]==3 and (word(b,130)>>14)==2
        first=unpack_from('<I',b,132)[0]
        blocks=1+(word(b,130)&0x3fff)
'''
if s.count(old)!=1:
    raise SystemExit('M184: expected canonical M110 FH2 map check changed')
target.write_text(s.replace(old,new))
# The separate SYSLOA386/INILOA disk test repeats the same old FH2
# assumption for all three images; it must accept the larger SYS.EXE too.
aux=root/'tests/test_sysloa386_disk_image.py'
a=aux.read_text()
if a.count(old)!=1:
    raise SystemExit('M184: expected SYSLOA386 FH2 map test changed')
aux.write_text(a.replace(old,new))
