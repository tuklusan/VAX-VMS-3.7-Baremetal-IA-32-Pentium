#!/usr/bin/env python3
"""Generate independently compilable, source-specific *partial* IA-32 RTL slices.
Original revision: 2f5bd967bc163bdb4aa9a5e4b1f9981d9995e752.
Never embeds or redistributes original copyrighted MACRO-32 sources.
Exact table values were transcribed by inspection from pinned .MAR byte data.
"""
from pathlib import Path
import json

ROOT=Path(__file__).resolve().parents[1]
REV='2f5bd967bc163bdb4aa9a5e4b1f9981d9995e752'
# Name -> (physical source lines, original public entry lines); includes .END.
data={
'LIBA2EREV':(147,[112]),'LIBE2AREV':(150,[115]),'LIBASCEBC':(152,[116]),'LIBEBCASC':(161,[125]),
'LIBASTINP':(177,[125]),'LIBADDX':(334,[163,281]),'LIBSIGNAL':(518,[187,242]),
'LIBINITIA':(247,[180]),'LIBTEMPLA':(126,[113]),'LIBCVTDF':(146,[135]),
'COBSPANC2':(109,[99]),'COBSPANC':(142,[107]),'COBDEEDIT':(107,[94]),'COBDHANDL':(142,[138]),
'LIBDAY':(206,[155]),'COBCVTPD':(171,[140]),'COBCVTPF':(171,[139]),
'COBCVTFP':(183,[141]),'COBCVTDP':(184,[142]),'COBCVTRQP':(174,[142]),
'COBCVTRFP':(184,[142]),'COBCVTRDP':(184,[142]),'COBCVTQP':(180,[148]),
'COBCVTPQ':(177,[146]),'COBCVTRPQ':(180,[148]),'COBCVTQD':(195,[146]),
'COBCVTQF':(195,[145]),'COBCVTFQ':(213,[154]),'COBCVTDQ':(213,[154]),
'COBCVTRFQ':(213,[154]),'COBCVTRDQ':(213,[154]),'COBINVUSE':(219,[140]),
'COBCNVOUT':(285,[163]),'COBDIVQ':(359,[154]),'COBEXPI':(876,[397,400]),
'BASMATNUL':(248,[148]),'BASMATRED':(359,[153]),'BASMATIDN':(364,[319]),
'BASMATINI':(399,[358]),'BASEDIT':(440,[168])
}
assert len(data)==40
TABLES={name:i for i,name in enumerate(('LIBA2EREV','LIBE2AREV','LIBASCEBC','LIBEBCASC','COBSPANC2','COBSPANC','COBDEEDIT'))}
CONVS={
'COBCVTPD':('PORT113_PACK_D',0),'COBCVTPF':('PORT113_PACK_F',0),
'COBCVTFP':('PORT113_F_PACK',0),'COBCVTDP':('PORT113_D_PACK',0),
'COBCVTRFP':('PORT113_F_PACK',1),'COBCVTRDP':('PORT113_D_PACK',1),
'COBCVTRQP':('PORT113_QUAD_PACK',1),'COBCVTQP':('PORT113_QUAD_PACK',0),
'COBCVTPQ':('PORT113_PACK_QUAD',0),'COBCVTRPQ':('PORT113_PACK_QUAD',1),
'COBCVTQD':('PORT113_QUAD_D',0),'COBCVTQF':('PORT113_QUAD_F',0),
'COBCVTFQ':('PORT113_F_QUAD',0),'COBCVTDQ':('PORT113_D_QUAD',0),
'COBCVTRFQ':('PORT113_F_QUAD',1),'COBCVTRDQ':('PORT113_D_QUAD',1)
}
assert len(CONVS)==16
special={
'LIBADDX': [('addx','int %s(const uint32_t *a,const uint32_t *b,uint32_t *out,size_t n){return port113_addx(a,b,out,n,0);}',163),
             ('subx','int %s(const uint32_t *a,const uint32_t *b,uint32_t *out,size_t n){return port113_addx(a,b,out,n,1);}',281)],
'LIBSIGNAL':[('stop','int %s(const uint32_t *raw_vax_frame,size_t words){(void)raw_vax_frame;(void)words;return PORT113_UNSUPPORTED;}',187),
             ('signal','int %s(const uint32_t *raw_vax_frame,size_t words){(void)raw_vax_frame;(void)words;return PORT113_UNSUPPORTED;}',242)],
'COBEXPI':[('expi_ose','int %s(int64_t base,int32_t exponent,int64_t *out){return port113_cobexpi(base,exponent,out);}',397),
           ('expi','int %s(int64_t base,int32_t exponent,int64_t *out){return port113_cobexpi(base,exponent,out);}',400)]
}
BODY={
'LIBASTINP':'int %s(uint32_t mode,unsigned in_ast,int *out){return port113_libastinp(mode,in_ast,out);}',
'LIBINITIA':'int %s(const struct port113_init_call *callbacks,size_t count,int *result){return port113_libinitia(callbacks,count,result);}',
'LIBTEMPLA':'int %s(const uint32_t *argument){return port113_libtempla(argument);}',
'LIBCVTDF':'int %s(const uint16_t *d,uint16_t *f){long double x; if(!d||!f)return PORT113_BAD_ARG; if(port109_decode(PORT_M107_D,d,&x)!=PORT109_OK)return PORT113_BAD_ARG; return port109_encode(PORT_M107_F,x,f)==PORT109_OK?PORT113_APPROX:PORT113_OVERFLOW;}',
'LIBDAY':'int %s(uint64_t vax_quadword_time,int32_t *day,uint32_t *centis){return port113_libday(vax_quadword_time,day,centis);}',
'COBDHANDL':'int %s(int (*use_proc)(void *,uint32_t),void *context,uint32_t arg,int *result){return port113_cobinvoke(use_proc,context,arg,result);}',
'COBINVUSE':'int %s(int (*use_proc)(void *,uint32_t),void *context,uint32_t arg,int *result){return port113_cobinvoke(use_proc,context,arg,result);}',
'COBCNVOUT':'int %s(int64_t value,char *dst,size_t length){return port113_cobcnvout(value,dst,length,1);}',
'COBDIVQ':'int %s(int64_t numerator,int64_t denominator,int64_t *quotient,int64_t *remainder){return port113_cobdivq(numerator,denominator,quotient,remainder);}',
'BASMATNUL':'int %s(struct port113_matrix *m){return port113_matrix_null(m);}',
'BASMATIDN':'int %s(struct port113_matrix *m){return port113_matrix_fill(m,1);}',
'BASMATRED':'int %s(struct port113_matrix *m,uint32_t rows,uint32_t cols){return port113_matrix_redim(m,rows,cols);}',
'BASMATINI':'int %s(struct port113_matrix *m,const uint16_t *value){return port113_matrix_initial(m,value);}',
'BASEDIT':'int %s(uint8_t *chars,size_t length,int upcase,int brackets){return port113_basedit(chars,length,upcase,brackets);}'
}
modules=[]
root=ROOT/'src/native_rtl_m113';root.mkdir(exist_ok=True)
for name,(nlines,entrylines) in data.items():
    stem=name.lower();stemc='port113_src_'+stem
    sections=[]
    if name in TABLES:
        sections=[(stemc,'int %s(uint8_t c,uint8_t *out){return port113_table('+str(TABLES[name])+',c,out);}',entrylines[0])]
        kind='exact-table'
    elif name in CONVS:
        typ,rounded=CONVS[name]
        sections=[(stemc,'int %s(const void *src,size_t digits,int32_t scale,void *dst,size_t cap){return port113_convert('+typ+',src,digits,scale,dst,cap,'+str(rounded)+');}',entrylines[0])]
        kind='bounded-conversion'
    elif name in special:
        sections=[('port113_src_'+suffix,code,line) for suffix,code,line in special[name]]
        kind='unresolved-frame' if name=='LIBSIGNAL' else 'bounded-arithmetic'
    else:
        assert name in BODY,name
        sections=[(stemc,BODY[name],entrylines[0])]
        kind='bounded-service'
    assert [line for sym,code,line in sections]==entrylines
    source='/* Native IA-32 source-specific partial entry; original: RTL/SRC/'+name+'.MAR, '+'; '.join('line '+str(q) for q in entrylines)+'.\n'
    source+=' * No DEC source is bundled. VAX calling mask, raised conditions, RTL integration not yet reproduced. */\n'
    source+='#include "../vms_rtl_m113.h"\n'
    for sym,fn,line in sections:
        assert '%s' in fn
        source+=fn%sym+'\n'
    (root/(stem+'.c')).write_text(source)
    modules.append({'name':name,'source':'RTL/SRC/'+name+'.MAR','lines':nlines,'kind':kind,
        'entries':[{'line':line,'symbol':sym,'status':'unresolved' if name=='LIBSIGNAL' else 'partial'} for sym,fn,line in sections]})
assert len(modules)==40
out={'revision':REV,'file_count':40,'modules':modules}
(ROOT/'tools/m113_originals.json').write_text(json.dumps(out,indent=2)+'\n')
# Publish exact IA-32 native C entry prototypes, not VAX ABI aliases.
header=ROOT/'src/vms_rtl_m113.h'
source=header.read_text()
start='/* Source-specific C declarations generated by tools/m113_modules.py */'
if start in source:source=source[:source.index(start)].rstrip()+'\n#endif\n'
assert source.endswith('#endif\n')
import re
protos=[]
for path in sorted(root.glob('*.c')):
    code=path.read_text()
    matches=re.findall(r'^int\s+(port113_src_\w+)\s*\([^\n]*?\)\s*\{',code,re.M)
    for sym in matches:
        sig=next(line.split('{',1)[0] for line in code.splitlines() if line.startswith('int '+sym+'('))
        protos.append(sig+';')
assert len(protos)==43,(len(protos),protos)
source=source[:-len('#endif\n')]+start+'\n'+'\n'.join(protos)+'\n#endif\n'
header.write_text(source)

print('M113 source-specific modules written:',len(modules),'exports:',sum(len(x['entries']) for x in modules))
