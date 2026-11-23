#include "vms_rtl_m110.h"
#include "vms_rtl_m109_x87.h"
/* Numerical convenience conversions use VAX F/D/G/H *raw word* adapters.
 * Never reinterpret VAX payloads as IEEE; do not emulate a VAX processor.
 */
static unsigned words(enum port_m107_fmt f){
    if(f==PORT_M107_F)return 2;
    if(f==PORT_M107_D||f==PORT_M107_G)return 4;
    if(f==PORT_M107_H)return 8;
    return 0;
}
static int acceptable(int st){return st==PORT109_OK;}
static void copy(unsigned n,const uint16_t *from,uint16_t *to){
    for(unsigned i=0;i<n;i++)to[i]=from[i];
}
static int bounded(long double x){return __builtin_isfinite(x) && x<1.0e6L && x> -1.0e6L;}
static const long double pi=3.1415926535897932384626433832795L;
static int decode2(enum port_m107_fmt f,const uint16_t *a,long double *r,long double *i){
    int s=port109_decode(f,a,r);
    if(s!=PORT109_OK)return s;
    return port109_decode(f,a+words(f),i);
}
static int encode2(enum port_m107_fmt f,long double r,long double i,uint16_t *out){
    uint16_t ra[8]={0},ia[8]={0};
    if(!__builtin_isfinite(r)||!__builtin_isfinite(i))return PORT109_RANGE;
    int s=port109_encode(f,r,ra);if(s!=PORT109_OK)return s;
    s=port109_encode(f,i,ia);if(s!=PORT109_OK)return s;
    unsigned n=words(f);copy(n,ra,out);copy(n,ia,out+n);
    return PORT109_APPROX;
}
int port110_complex(enum port_m107_fmt f,enum port110_complex_op op,
                    const uint16_t *a,uint16_t *out){
    if(!words(f)||!a||!out||op>PORT110_CCOS)return PORT109_BAD_ARG;
    long double r=0.0L,i=0.0L;
    int st=decode2(f,a,&r,&i);if(!acceptable(st))return st;
    long double x=0.0L,y=0.0L;
    if((op==PORT110_CSIN||op==PORT110_CCOS) && !bounded(r))return PORT109_PARTIAL;
    switch(op){
    case PORT110_CEXP:{
        if(!bounded(i))return PORT109_PARTIAL;
        long double e=port109_x87_exp(r);
        x=e*port109_x87_cos(i);y=e*port109_x87_sin(i);
        break; }
    case PORT110_CLOG:{
        if(r==0.0L&&i==0.0L)return PORT109_DOMAIN;
        x=port109_x87_ln(port109_x87_sqrt(r*r+i*i));
        y=port109_x87_atan2(i,r);break; }
    case PORT110_CSQRT:{
        long double h=port109_x87_sqrt(r*r+i*i);
        x=port109_x87_sqrt((h+r)*0.5L);
        y=port109_x87_sqrt((h-r)*0.5L);
        if(i<0.0L)y=-y;
        break; }
    case PORT110_CSIN:case PORT110_CCOS:{
        long double sn=port109_x87_sin(r),co=port109_x87_cos(r);
        long double e=port109_x87_exp(i),inv=port109_x87_exp(-i);
        long double ch=(e+inv)*0.5L,sh=(e-inv)*0.5L;
        if(op==PORT110_CSIN){x=sn*ch;y=co*sh;}
        else{x=co*ch;y=-sn*sh;}
        break; }
    }
    return encode2(f,x,y,out);
}
int port110_atanh(enum port_m107_fmt f,const uint16_t *a,uint16_t *out){
    if(!a||!out||!words(f))return PORT109_BAD_ARG;
    long double x;
    int st=port109_decode(f,a,&x);if(st!=PORT109_OK)return st;
    if(x<=-1.0L||x>=1.0L)return PORT109_DOMAIN;
    if(x==0.0L){for(unsigned i=0;i<words(f);i++)out[i]=0;return PORT109_OK;}
    /* Domain and zero identity exact; other results approximate x87 ln. */
    long double v=0.5L*port109_x87_ln((1.0L+x)/(1.0L-x));
    uint16_t temp[8]={0};st=port109_encode(f,v,temp);
    if(st!=PORT109_OK)return st;
    copy(words(f),temp,out);return PORT109_APPROX;
}
int port110_sincos(enum port_m107_fmt f,int degrees,const uint16_t *a,
                   uint16_t *s,uint16_t *c){
    if(!a||!s||!c||!words(f)||((degrees!=0)&&(degrees!=1))||s==c)
        return PORT109_BAD_ARG;
    long double x;
    int st=port109_decode(f,a,&x);if(st!=PORT109_OK)return st;
    if(degrees)x*=pi/180.0L;
    if(!bounded(x))return PORT109_PARTIAL;
    uint16_t stmp[8]={0},ctmp[8]={0};
    st=port109_encode(f,port109_x87_sin(x),stmp);if(st!=PORT109_OK)return st;
    st=port109_encode(f,port109_x87_cos(x),ctmp);if(st!=PORT109_OK)return st;
    copy(words(f),stmp,s);copy(words(f),ctmp,c);
    return PORT109_APPROX;
}
int port110_poly(enum port_m107_fmt f,const uint16_t *x,
                 unsigned order,const uint16_t *coeff,uint16_t *out){
    if(!words(f)||!x||!coeff||!out||order>32)return PORT109_BAD_ARG;
    long double xx=0.0L,acc=0.0L;
    int st=port109_decode(f,x,&xx);if(st!=PORT109_OK)return st;
    unsigned n=words(f);
    st=port109_decode(f,coeff+order*n,&acc);if(st!=PORT109_OK)return st;
    /* VAX POLYx coefficients are ordered C0,C1,...,Cn. */
    for(unsigned i=order;i>0;i--){
        long double c=0.0L;
        st=port109_decode(f,coeff+(i-1u)*n,&c);if(st!=PORT109_OK)return st;
        acc=acc*xx+c;
        if(!__builtin_isfinite(acc))return PORT109_RANGE;
    }
    uint16_t tmp[8]={0};st=port109_encode(f,acc,tmp);if(st!=PORT109_OK)return st;
    copy(n,tmp,out);return PORT109_APPROX;
}
int port110_emod(enum port_m107_fmt f,const uint16_t *mulr,
                 uint16_t extension,const uint16_t *muld,int32_t *integral,
                 uint16_t *fraction){
    if(!words(f)||!mulr||!muld||!integral||!fraction)return PORT109_BAD_ARG;
    /* VAX extended multiplication includes an exponent-dependent 16-bit
     * fraction; nonzero extension is NOT treated as an ordinary multiply. */
    if(extension)return PORT109_PARTIAL;
    long double a=0.0L,b=0.0L;
    int st=port109_decode(f,mulr,&a);if(st!=PORT109_OK)return st;
    st=port109_decode(f,muld,&b);if(st!=PORT109_OK)return st;
    long double z=a*b;
    if(!__builtin_isfinite(z)||z>2147483647.0L||z< -2147483648.0L)
        return PORT109_RANGE;
    int32_t i=(int32_t)z; /* VAX EMOD integer truncates toward zero. */
    uint16_t tmp[8]={0};st=port109_encode(f,z-(long double)i,tmp);
    if(st!=PORT109_OK)return st;
    copy(words(f),tmp,fraction);*integral=i;
    return PORT109_APPROX;
}
int port110_convert(enum port_m107_fmt src,enum port_m107_fmt dst,
                    const uint16_t *a,uint16_t *out){
    if(!words(src)||!words(dst)||!a||!out)return PORT109_BAD_ARG;
    long double v;
    int st=port109_decode(src,a,&v);if(st!=PORT109_OK)return st;
    uint16_t tmp[8]={0};st=port109_encode(dst,v,tmp);if(st!=PORT109_OK)return st;
    copy(words(dst),tmp,out);return PORT109_APPROX;
}
int port110_float_to_int(enum port_m107_fmt f,const uint16_t *a,
                         unsigned bits,int32_t *result){
    if(!words(f)||!a||!result||(bits!=16u&&bits!=32u))return PORT109_BAD_ARG;
    long double x;
    int st=port109_decode(f,a,&x);if(st!=PORT109_OK)return st;
    if(bits==16u&&(x>32767.0L||x< -32768.0L))return PORT109_RANGE;
    if(bits==32u&&(x>2147483647.0L||x< -2147483648.0L))return PORT109_RANGE;
    *result=(int32_t)x;return PORT109_APPROX;
}
int port110_int_to_float(enum port_m107_fmt f,int32_t value,uint16_t *out){
    if(!words(f)||!out)return PORT109_BAD_ARG;
    uint16_t tmp[8]={0};int st=port109_encode(f,(long double)value,tmp);
    if(st!=PORT109_OK)return st;
    copy(words(f),tmp,out);return PORT109_APPROX;
}
/* Strict single-byte alpha/digit/space EBCDIC mappings, independent of
 * DEC source tables; no dynamic descriptor/escape processing is claimed. */
static int map_ascii(uint8_t a,uint8_t *b){
    if(a>='A'&&a<='I'){*b=(uint8_t)(0xc1u+a-'A');return 1;}
    if(a>='J'&&a<='R'){*b=(uint8_t)(0xd1u+a-'J');return 1;}
    if(a>='S'&&a<='Z'){*b=(uint8_t)(0xe2u+a-'S');return 1;}
    if(a>='a'&&a<='i'){*b=(uint8_t)(0x81u+a-'a');return 1;}
    if(a>='j'&&a<='r'){*b=(uint8_t)(0x91u+a-'j');return 1;}
    if(a>='s'&&a<='z'){*b=(uint8_t)(0xa2u+a-'s');return 1;}
    if(a>='0'&&a<='9'){*b=(uint8_t)(0xf0u+a-'0');return 1;}
    if(a==' '){*b=0x40u;return 1;}
    return 0;
}
static int map_ebcdic(uint8_t a,uint8_t *b){
    if(a>=0xc1u&&a<=0xc9u){*b=(uint8_t)('A'+a-0xc1u);return 1;}
    if(a>=0xd1u&&a<=0xd9u){*b=(uint8_t)('J'+a-0xd1u);return 1;}
    if(a>=0xe2u&&a<=0xe9u){*b=(uint8_t)('S'+a-0xe2u);return 1;}
    if(a>=0x81u&&a<=0x89u){*b=(uint8_t)('a'+a-0x81u);return 1;}
    if(a>=0x91u&&a<=0x99u){*b=(uint8_t)('j'+a-0x91u);return 1;}
    if(a>=0xa2u&&a<=0xa9u){*b=(uint8_t)('s'+a-0xa2u);return 1;}
    if(a>=0xf0u&&a<=0xf9u){*b=(uint8_t)('0'+a-0xf0u);return 1;}
    if(a==0x40u){*b=' ';return 1;}
    return 0;
}
static int translate(const uint8_t *s,unsigned n,uint8_t *d,unsigned cap,
                     int(*mapper)(uint8_t,uint8_t *)){
    if((!s&&n)||(!d&&n)||cap<n)return PORT109_BAD_ARG;
    /* All-or-nothing: a rejected byte never leaves a partial string. */
    for(unsigned i=0;i<n;i++){uint8_t temp;
        if(!mapper(s[i],&temp))return PORT109_PARTIAL;}
    for(unsigned i=0;i<n;i++){uint8_t temp=0;mapper(s[i],&temp);d[i]=temp;}
    return PORT109_APPROX;
}
int port110_asc_to_ebc(const uint8_t *s,unsigned n,uint8_t *d,unsigned cap){
    return translate(s,n,d,cap,map_ascii);
}
int port110_ebc_to_asc(const uint8_t *s,unsigned n,uint8_t *d,unsigned cap){
    return translate(s,n,d,cap,map_ebcdic);
}
void port110_ef_init(struct port110_ef_pool *p){
    if(p)p->busy=UINT64_C(0xffffffff00000000);
}
int port110_ef_get(struct port110_ef_pool *p,unsigned *n){
    if(!p||!n)return PORT109_BAD_ARG;
    for(unsigned k=63;k>=32;k--){
        uint64_t mask=UINT64_C(1)<<(63u-k);
        if(!(p->busy&mask)){p->busy|=mask;*n=k;return PORT109_OK;}
    }
    *n=~0u;return PORT109_PARTIAL;
}
static int ef_valid(unsigned n){return (n>=1u&&n<=23u) || (n>=32u&&n<=63u);}
int port110_ef_free(struct port110_ef_pool *p,unsigned n){
    if(!p||!ef_valid(n))return PORT109_BAD_ARG;
    uint64_t mask=UINT64_C(1)<<(63u-n);
    if(!(p->busy&mask))return PORT109_PARTIAL;
    p->busy &= ~mask;return PORT109_OK;
}
int port110_ef_reserve(struct port110_ef_pool *p,unsigned n){
    if(!p||!ef_valid(n))return PORT109_BAD_ARG;
    uint64_t mask=UINT64_C(1)<<(63u-n);
    if(p->busy&mask)return PORT109_PARTIAL;
    p->busy |= mask;return PORT109_OK;
}
void port110_lun_init(struct port110_lun_pool *p){if(p)p->busy=UINT32_C(0xfff00000);}
int port110_lun_get(struct port110_lun_pool *p,unsigned *n){
    if(!p||!n)return PORT109_BAD_ARG;
    for(unsigned k=119;k>=100;k--){
        uint32_t mask=UINT32_C(1)<<(119u-k);
        if(!(p->busy&mask)){p->busy|=mask;*n=k;return PORT109_OK;}
    }
    *n=~0u;return PORT109_PARTIAL;
}
int port110_lun_free(struct port110_lun_pool *p,unsigned n){
    if(!p||n<100u||n>119u)return PORT109_BAD_ARG;
    uint32_t mask=UINT32_C(1)<<(119u-n);
    if(!(p->busy&mask))return PORT109_PARTIAL;
    p->busy &= ~mask;return PORT109_OK;
}
