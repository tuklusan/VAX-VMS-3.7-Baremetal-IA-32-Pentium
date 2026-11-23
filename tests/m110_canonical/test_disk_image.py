from pathlib import Path
from struct import unpack_from

# Read the Files-11 INDEXF map and each file header. SYS.EXE grows as
# original-source code is added; hardcoded LBN 115 is not a Files-11 rule.
volume=(Path(__file__).resolve().parents[1]/'portdisk.img').read_bytes()
BLOCK=512
TOTAL=8*16*63
assert len(volume)==TOTAL*BLOCK

def block(lbn):
    assert 0<=lbn<TOTAL
    return volume[lbn*BLOCK:(lbn+1)*BLOCK]

def word(b,n):
    return unpack_from('<H',b,n)[0]

def checksum(b,at):
    return sum(word(b,p) for p in range(0,at,2))&0xffff

def valid_header(b,fid):
    assert b[:2]==b'\x26\x41'
    assert word(b,8)==fid and word(b,10)==fid
    assert checksum(b,510)==word(b,510)
    assert b[58]==2 # one format-1 extent, as installed by native image tools
    first=word(b,132)
    blocks=1+(word(b,130)&0xff)
    assert 0<first<TOTAL and first+blocks<=TOTAL
    return first,blocks

assert block(0)[510:512]==b'\x55\xaa'
for home_lbn in (3,6):
    h=block(home_lbn)
    assert unpack_from('<I',h,0)[0]==home_lbn
    assert checksum(h,58)==word(h,58)
    assert checksum(h,510)==word(h,510)
index=block(13)
assert index[:2]==b'\x26\x41'
assert word(index,8)==1
assert checksum(index,510)==word(index,510)
assert word(index,130)==0x4017 and word(index,132)==0
assert word(index,134)==0x4002
index_extension=word(index,136)
assert 24<=index_extension<TOTAL-2
index_bitmap=block(12)
assert (word(index_bitmap,0)&0x1fff)==0x1fff
directory=block(30)
assert all(name in directory for name in (b'SYS.EXE',b'SYSLOA386.EXE',b'INILOA.EXE'))
# FID11 resides in INDEXF's initial mapped extent (LBN23).
# FIDs12 and 13 use contiguous blocks 0 and 1 of its later extent.
seen=[]
for fid,header_lbn in ((11,23),(12,index_extension),(13,index_extension+1)):
    data_lbn,blocks=valid_header(block(header_lbn),fid)
    seen.append((data_lbn,data_lbn+blocks))
    elf=block(data_lbn)
    assert elf[:5]==b'\x7fELF\x01'
    assert word(elf,18)==3 # ELF32 IA-32 CPU
for i,(start,end) in enumerate(seen):
    for j,(a,b) in enumerate(seen):
        assert i==j or end<=a or b<=start
print('Files-11 INDEXF-mapped SYS, SYSLOA386 and INILOA ELF32 images PASS')
