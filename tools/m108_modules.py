#!/usr/bin/env python3
"""38 pinned VMS 3.7 RTL modules, each saved as its own translated IA-32 unit.

Reference: calmsacibis995/vms37-src@2f5bd967bc163bdb4aa9a5e4b1f9981d9995e752
No DEC original source is copied.  These are partial native implementations;
unsupported numerical domains return PORT108_PARTIAL, never a guessed answer.
"""
from pathlib import Path
import os
ROOT=Path(__file__).resolve().parents[1]
DIR=ROOT/'src/native_rtl_m108'
DIR.mkdir(parents=True,exist_ok=True)
modules={}
def add(name,body,detail):
    assert name not in modules
    modules[name]=(body.strip(),detail)
add('mthabs','''
int port108_mthabs_i2(int16_t x,int16_t *out){return port108_integer_abs16(x,out);}
int port108_mthabs_i4(int32_t x,int32_t *out){return port108_integer_abs32(x,out);}
int port108_mthabs_f(enum port_m107_fmt f,const uint16_t *x,uint16_t *out){return port108_float_transform(f,x,out,PORT108_ABS);}
''','IIABS, JIABS, ABS, DABS, GABS and HABS; INT_MIN wraps as MNEG.')
for name,fmt,operation in [('mthaint','F','TRUNC'),('mthanint','F','NEAREST'),('mthfloor','F','FLOOR'),('mthdint','D','TRUNC'),('mthgint','G','TRUNC'),('mthhint','H','TRUNC'),('mthdfloor','D','FLOOR'),('mthgfloor','G','FLOOR'),('mthhfloor','H','FLOOR'),('mthdnint','D','NEAREST'),('mthgnint','G','NEAREST'),('mthhnint','H','NEAREST')]:
    add(name,f'''int port108_{name}(const uint16_t *v,uint16_t *out){{
    return port108_float_transform(PORT_M107_{fmt},v,out,PORT108_{operation});
}}''','Native VAX '+fmt+' integral rounding; original register-entry/PSL variants remain unbound.')
for name,fmt in [('mthamod','F'),('mthdmod','D'),('mthgmod','G'),('mthhmod','H')]:
    add(name,f'''int port108_{name}(const uint16_t *a,const uint16_t *b,uint16_t *out){{
    return port108_float_mod(PORT_M107_{fmt},a,b,out);
}}''','Exact VAX '+fmt+' integer-valued subset only; fractional EMOD and signaling still unresolved.')
add('mthmod','''
int port108_mthmod_i2(int16_t a,int16_t b,int16_t *out){return port108_integer_mod16(a,b,out);}
int port108_mthmod_i4(int32_t a,int32_t b,int32_t *out){return port108_integer_mod32(a,b,out);}
''','IMOD/JMOD signed remainder; divide-by-zero is an explicit status, not a VAX condition.')
add('mthdim','''
int port108_mthdim_i2(int16_t a,int16_t b,int16_t *out){return port108_integer_dim16(a,b,out);}
int port108_mthdim_i4(int32_t a,int32_t b,int32_t *out){return port108_integer_dim32(a,b,out);}
int port108_mthdim_float(enum port_m107_fmt f,const uint16_t *a,const uint16_t *b,uint16_t *out){return port108_float_dim(f,a,b,out);}
''','IIDIM/JIDIM exact wrapped subtraction; F/D/G/H DIM only exact integral bounded subset.')
for name,fmt in [('mthdprod','D'),('mthgprod','G')]:
    add(name,f'''int port108_{name}(const uint16_t a[2],const uint16_t b[2],uint16_t *out){{
    return port108_float_product(PORT_M107_{fmt},a,b,out);
}}''','Native exact 24-bit VAX F significand multiplication widened to '+fmt+' format.')
add('mthbitops','''
int port108_mthbitops_i2(unsigned operation,uint16_t a,uint16_t b,uint16_t *out){
 return port108_math_bit16(operation,a,b,out);
}
int port108_mthbitops_i4(unsigned operation,uint32_t a,uint32_t b,uint32_t *out){
 return port108_math_bit32(operation,a,b,out);
}
''','All ten integer AND, OR, XOR, NOT and shift entry points; no VAX entry frame.')
for name,fmt in [('mthcabs','F'),('mthcdabs','D'),('mthcgabs','G')]:
    add(name,f'''int port108_{name}(const uint16_t *pair,uint16_t *out){{
 return port108_float_cabs(PORT_M107_{fmt},pair,out);
}}''','Exact integer Pythagorean subdomain of complex magnitude; arbitrary SQRT remains unresolved.')
add('mthrandom','''int port108_mthrandom(uint32_t *seed,uint16_t out[2]){
 return port108_random_mth(seed,out);
}''','Original 69069 state transition and high-24 fraction as VAX F encoding.')
add('forrandom','''int port108_forrandom(uint16_t *high,uint16_t *low,uint16_t out[2]){
 return port108_random_fortran(high,low,out);
}''','Original 65539 two-word seed transition; F fraction precision and first-call handling partial.')
for name,method,tail in [('libinsqhi','insert','0'),('libinsqti','insert','1'),('libremqhi','remove','0'),('libremqti','remove','1')]:
    if method=='insert':
        body=f'''int port108_{name}(struct port108_queue *q,uint32_t entry){{
 return port108_queue_insert(q,entry,{tail});
}}'''
    else:
        body=f'''int port108_{name}(struct port108_queue *q,uint32_t *removed){{
 return port108_queue_remove(q,removed,{tail});
}}'''
    add(name,body,'Source-ordered 32-bit queue links and empty/one statuses; interlocked retries not yet live.')
add('strmovq','''int port108_strmovq(const uint8_t *src,uint8_t *dst){return port108_movq(src,dst);}''','MOVQ copies all eight bytes with overlap-safe scratch.')
add('libmovtc','''int port108_libmovtc(const uint8_t *src,size_t ns,const uint8_t table[256],uint8_t fill,uint8_t *dest,size_t nd){
 return port108_movtc(src,ns,table,fill,dest,nd);
}''','Byte translation plus fill for fixed descriptors; dynamic descriptor and overlap unresolved.')
add('libmovtuc','''int port108_libmovtuc(const uint8_t *src,size_t ns,const uint8_t table[256],uint8_t escape,uint8_t fill,uint8_t *dest,size_t nd,size_t *index){
 return port108_movtuc(src,ns,table,escape,fill,dest,nd,index);
}''','MOVTUC escape index with translated table, fixed destination only.')
add('libcrc','''uint32_t port108_libcrc(uint32_t polynomial,uint32_t initial,const uint8_t *data,size_t bytes){
 return port108_crc_reference(polynomial,initial,data,bytes);
}''','Reference reflected polynomial CRC, original 16-table VAX CRC hardware/table format still unresolved.')
add('libemul','''int port108_libemul(int32_t multiplier,int32_t multiplicand,int32_t addend,int64_t *result){
 return port108_emul(multiplier,multiplicand,addend,result);
}''','EMUL exact signed 32x32 plus 32-bit addend to 64-bit result.')
add('libediv','''int port108_libediv(int32_t divisor,int64_t dividend,int32_t *quot,int32_t *rem){
 return port108_ediv(divisor,dividend,quot,rem);
}''','EDIV quotient/remainder; VAX INTDIV/INTOVF condition handler integration pending.')
add('libgetopc','''int port108_libgetopc(const uint8_t *instruction,size_t readable,uint16_t *opcode){
 return port108_get_opcode(instruction,readable,opcode);
}''','VAX one- or two-byte opcode decode; signal handler and mapped access unbound.')
assert len(modules)==38,len(modules)
for name,(body,detail) in modules.items():
    p=DIR/(name+'.c')
    with p.open('w',encoding='utf-8',newline='\n') as fd:
        fd.write(f'/* Original: RTL/SRC/{name.upper()}.MAR at pinned DEC reference.\n'
                 f' * {detail}\n * Host-tested IA-32 subset, NOT a complete translated VMS RTL image.\n'
                 ' * No original DEC source text is included.\n */\n'
                 '#include "../vms_rtl_m108.h"\n'+body+'\n')
        fd.flush();os.fsync(fd.fileno())
print('Saved original-specific native IA-32 C files:',len(modules))
