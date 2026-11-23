#!/usr/bin/env python3
"""Independent original-to-native translation file list; writes each file durably.

The small PC wrappers preserve callable entry points and source-specific
original format/width/operation, using common IA-32 native primitives.
These are partial RTL modules, not guest-loaded services or full VMS ABIs.
"""
from pathlib import Path
import os

ROOT=Path(__file__).resolve().parent.parent
DIR=ROOT/'src'/'native_rtl_m107'
DIR.mkdir(exist_ok=True)

specs=[]
for name,fmt,bits in [
    ('mthiidnnt','D',16),('mthiignnt','G',16),('mthiihnnt','H',16),
    ('mthjidnnt','D',32),('mthjignnt','G',32),('mthjihnnt','H',32),
    ('mthinint','F',16),('mthjnint','F',32)]:
    specs.append((name,f'''int port_m107_{name}(const uint16_t *a,int32_t *r){{
    return port_m107_float_rounded(PORT_M107_{fmt},a,{bits}u,r);
}}''','The VAX CVTR result keeps its low result bits on integer overflow; callers receive status.'))
for name,width in [('mthiisign',16),('mthjisign',32)]:
    specs.append((name,f'''int port_m107_{name}(int{width}_t magnitude,int{width}_t sign,int{width}_t *out){{
    return port_m107_int{width}_sign(magnitude,sign,out);
}}''','The VAX MNEG operation wraps at the narrow signed minimum; no host signed overflow.'))
for name,width,maximum in [('mthimax0',16,1),('mthimin0',16,0),('mthjmax0',32,1),('mthjmin0',32,0)]:
    specs.append((name,f'''int port_m107_{name}(const int{width}_t * const *a,size_t n,int{width}_t *out){{
    return port_m107_int{width}_extreme(a,n,{maximum},out);
}}''','Source count is the VAX low byte of the argument count; helper rejects zero and >255.'))
for name,fmt in [('mthsign','F'),('mthdsign','D'),('mthgsign','G'),('mthhsign','H')]:
    specs.append((name,f'''int port_m107_{name}(const uint16_t *a,const uint16_t *b,uint16_t *out){{
    return port_m107_float_sign(PORT_M107_{fmt},a,b,out);
}}''','Sign and exponent read from the low-addressed VAX word, not a host IEEE float.'))
for name,fmt,maximum in [
    ('mthmax1','F',1),('mthmin1','F',0),('mthdmax1','D',1),('mthdmin1','D',0),
    ('mthgmax1','G',1),('mthgmin1','G',0),('mthhmax1','H',1),('mthhmin1','H',0)]:
    warning='The original HMIN source uses MOVG/CMPG and an uninitialized R1: executable intended H comparison is not proven equivalent.' if name=='mthhmin1' else 'Only the base VAX format result is implemented; additional integer-return entry variants are unresolved.'
    specs.append((name,f'''int port_m107_{name}(const uint16_t * const *a,size_t n,uint16_t *out){{
    return port_m107_float_extreme(PORT_M107_{fmt},a,n,out,{maximum});
}}''',warning))
for name,fmt in [('mthconjg','F'),('mthdconjg','D'),('mthgconjg','G')]:
    specs.append((name,f'''int port_m107_{name}(const uint16_t *pair,uint16_t *out){{
    return port_m107_float_conj(PORT_M107_{fmt},pair,out);
}}''','Copies the real component and negates the imaginary sign, preserving zero.'))
specs.append(('mthsgn','''int port_m107_mthsgn(const uint16_t *v,int *result){
    static const uint16_t zero[2]={0,0};
    if(!result)return PORT_M107_BAD_ARG;
    return port_m107_float_cmp(PORT_M107_F,v,zero,result);
}''','Returns -1, 0 or +1 for VAX F signs; the R1 assembly service alias remains unresolved.'))
specs.extend([
    ('comr50wd','''int port_m107_comr50wd(const char *ascii,size_t n,uint16_t *out,size_t *used){
    return port_m107_rad50_word(ascii,n,out,used);
}''','Three-character native RADIX-50 word packer; invalid character scan semantics incomplete.'),
    ('comrad50','''int port_m107_comrad50(const char *ascii,uint32_t *out){
    return port_m107_rad50_six(ascii,out);
}''','Six-character RADIX-50 is two 16-bit words packed in DEC high-word first order.'),
    ('comirad50','''int port_m107_comirad50(const char *ascii,size_t n,uint16_t *out,size_t capacity,size_t *consumed){
    return port_m107_irad50(ascii,n,out,capacity,consumed);
}''','Caller-sized RADIX-50 array; full VAX descriptor arguments not yet integrated.'),
    ('comr50asc','''int port_m107_comr50asc(const uint16_t *encoded,size_t n,char *ascii,size_t capacity){
    return port_m107_r50asc(encoded,n,ascii,capacity);
}''','RADIX-50 inverse for all 40 codes, including unused code 29.'),
])
assert len(specs)==34
for name, body, warning in specs:
    filename=DIR/(name+'.c')
    doc=f'''/* Original: RTL/SRC/{name.upper()}.MAR (read-only DEC reference).
 * Native IA-32 operation: {warning}
 * Not a complete VAX calling-convention port and not a guest-loaded RTL.
 * No original source text is distributed here.
 */
#include "../vms_rtl_m107.h"
{body}
'''
    with filename.open('w',encoding='utf-8',newline='\n') as f:
        f.write(doc)
        f.flush()
        os.fsync(f.fileno())
print('Native source files written separately:',len(specs))
for name,_,_ in specs:print(name)
