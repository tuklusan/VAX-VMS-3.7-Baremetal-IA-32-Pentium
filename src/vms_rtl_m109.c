#include "vms_rtl_m109.h"
/* Native freestanding IA-32 x87 implementation of VMS 3.7 RTL mathematical entry-point
 * SUBSETS. The original VAX hardware is not emulated. Explicit FP word
 * adapters preserve VAX on-disk representation at the interface. This code
 * is deliberately NOT linked into SYS.EXE until its exception/PSL semantics,
 * condition signaling, and 113-bit H precision can be supplied. */
#include "vms_rtl_m109_x87.h"

static unsigned count(enum port_m107_fmt f){
    return f==PORT_M107_F ? 2u : f==PORT_M107_H ? 8u : 4u;
}
static unsigned radix_shift(enum port_m107_fmt f){
    return f==PORT_M107_G ? 4u : f==PORT_M107_H ? 0u : 7u;
}
static unsigned bias(enum port_m107_fmt f){
    return f==PORT_M107_G ? 1024u : f==PORT_M107_H ? 16384u : 128u;
}
static int fmt_valid(enum port_m107_fmt f){
    return f==PORT_M107_F||f==PORT_M107_D||f==PORT_M107_G||f==PORT_M107_H;
}
static unsigned bit_count(enum port_m107_fmt f){
    return 1u+radix_shift(f)+16u*(count(f)-1u);
}
static unsigned bit_at(enum port_m107_fmt f,const uint16_t *v,unsigned i){
    if(i==0)return 1u;
    unsigned s=radix_shift(f);
    if(i<=s)return (v[0]>>(s-i))&1u;
    i-=s+1u;
    return (v[1u+i/16u]>>(15u-i%16u))&1u;
}
static void set_bit(enum port_m107_fmt f,uint16_t *v,unsigned i,int on){
    if(i==0)return;
    unsigned s=radix_shift(f),wi=0u,p=0u;
    if(i<=s)p=s-i;
    else{i-=s+1u;wi=1u+i/16u;p=15u-i%16u;}
    uint16_t mask=(uint16_t)(1u<<p);
    if(on)v[wi]|=mask;else v[wi]&=(uint16_t)~mask;
}
static void raw_zero(enum port_m107_fmt f,uint16_t *out){
    for(unsigned i=0;i<count(f);i++)out[i]=0u;
}
static void raw_copy(enum port_m107_fmt f,const uint16_t *in,uint16_t *out){
    for(unsigned i=0;i<count(f);i++)out[i]=in[i];
}
int port109_decode(enum port_m107_fmt f,const uint16_t *raw,long double *out){
    if(!raw||!out||!fmt_valid(f))return PORT109_BAD_ARG;
    if(!(raw[0]&0x7fffu) && (raw[0]&0x8000u))return PORT109_RESERVED;
    unsigned e=(raw[0]&0x7fffu)>>radix_shift(f);
    if(!e){*out=0.0L;return PORT109_OK;}
    unsigned p=bit_count(f);
    /* x87 has 64 fraction/significand bits. Low H bits are significant,
     * and must NOT silently be dropped as a floating approximation. */
    if(f==PORT_M107_H){
        for(unsigned i=64u;i<p;i++)if(bit_at(f,raw,i))return PORT109_PARTIAL;
    }
    long double mantissa=0.5L,weight=0.25L;
    for(unsigned i=1u;i<p;i++){
        if(bit_at(f,raw,i))mantissa+=weight;
        weight*=0.5L;
    }
    int exponent=(int)e-(int)bias(f);
    long double val=port109_x87_scale(mantissa,exponent);
    if(!__builtin_isfinite(val)||val==0.0L)return PORT109_PARTIAL;
    *out=(raw[0]&0x8000u)?-val:val;
    return PORT109_OK;
}
int port109_encode(enum port_m107_fmt f,long double val,uint16_t *out){
    if(!out||!fmt_valid(f))return PORT109_BAD_ARG;
    if(!__builtin_isfinite(val))return PORT109_RANGE;
    if(val==0.0L){raw_zero(f,out);return PORT109_OK;}
    int negative=val<0.0L;
    if(negative)val=-val;
    int e=0;
    long double normalized=port109_x87_extract(val,&e);  /* [0.5,1) */
    int field=e+(int)bias(f);
    if(field<=0||field >= (1<<(15u-radix_shift(f))))return PORT109_RANGE;
    unsigned p=bit_count(f);
    uint16_t tmp[8]={0};
    tmp[0]=(uint16_t)(((unsigned)field<<radix_shift(f))|
                      (negative?0x8000u:0u));
    long double fraction=normalized*2.0L-1.0L;
    for(unsigned i=1u;i<p;i++){
        fraction*=2.0L;
        if(fraction>=1.0L){
            set_bit(f,tmp,i,1);
            fraction-=1.0L;
        }
    }
    /* IEEE x87 arithmetic is used to calculate this candidate; nearest
     * even quantization is a best-effort adapter, NOT original polynomial
     * rounding. An H-float output has 49 zero low bits beyond x87 precision. */
    if(fraction>0.5L ||
       (fraction==0.5L && bit_at(f,tmp,p-1u))){
        unsigned i=p-1u;
        for(;;){
            if(i==0u){
                ++field;
                if(field >= (1<<(15u-radix_shift(f))))return PORT109_RANGE;
                for(unsigned w=0;w<count(f);w++)tmp[w]=0;
                tmp[0]=(uint16_t)(((unsigned)field<<radix_shift(f))|
                                  (negative?0x8000u:0u));
                break;
            }
            if(!bit_at(f,tmp,i)){set_bit(f,tmp,i,1);break;}
            set_bit(f,tmp,i,0);--i;
        }
    }
    raw_copy(f,tmp,out);
    return PORT109_OK;
}
static int finish(enum port_m107_fmt f,long double y,uint16_t *out){
    /* Output bytes change atomically only after VAX range/format checks. */
    uint16_t tmp[8]={0};
    int st=port109_encode(f,y,tmp);
    if(st!=PORT109_OK)return st;
    raw_copy(f,tmp,out);
    return PORT109_APPROX;
}
static const long double port109_pi=
    3.1415926535897932384626433832795028841971693993751L;
static int calculate(enum port_m107_fmt f,enum port109_function op,
                     const uint16_t *arg,int degrees,uint16_t *out){
    if(!out||!arg||!fmt_valid(f)||op>PORT109_LOG2)return PORT109_BAD_ARG;
    long double x=0.0L;
    int st=port109_decode(f,arg,&x);
    if(st!=PORT109_OK)return st;
    if((op==PORT109_ACOS||op==PORT109_ASIN) && (x>1.0L||x< -1.0L))
        return PORT109_DOMAIN;
    if((op==PORT109_LOG||op==PORT109_LOG10||op==PORT109_LOG2) && x<=0.0L)
        return PORT109_DOMAIN;
    if(op==PORT109_SQRT && x<0.0L)return PORT109_DOMAIN;
    if(degrees && op!=PORT109_ACOS && op!=PORT109_ASIN && op!=PORT109_ATAN)
        return PORT109_BAD_ARG;
    /* Exact identities are independent of the DEC polynomial tables. */
    if((x==0.0L && (op==PORT109_ASIN||op==PORT109_ATAN||
                     op==PORT109_SQRT||op==PORT109_SINH||op==PORT109_TANH)) ||
       (x==1.0L && (op==PORT109_ACOS||op==PORT109_LOG||
                     op==PORT109_LOG10||op==PORT109_LOG2))){
        raw_zero(f,out);return PORT109_OK;
    }
    if(x==1.0L && op==PORT109_SQRT){
        raw_copy(f,arg,out);return PORT109_OK;
    }
    if(x==0.0L && (op==PORT109_EXP||op==PORT109_COSH)){
        return port109_encode(f,1.0L,out);
    }
    long double y=0.0L;
    switch(op){
    case PORT109_ACOS:y=port109_x87_atan2(port109_x87_sqrt(1.0L-x*x),x);break;
    case PORT109_ASIN:y=port109_x87_atan2(x,port109_x87_sqrt(1.0L-x*x));break;
    case PORT109_ATAN:y=port109_x87_atan2(x,1.0L);break;
    case PORT109_LOG:y=port109_x87_ln(x);break;
    case PORT109_LOG10:y=port109_x87_log10(x);break;
    case PORT109_LOG2:y=port109_x87_log2(x);break;
    case PORT109_EXP:y=port109_x87_exp(x);break;
    case PORT109_SQRT:y=port109_x87_sqrt(x);break;
    case PORT109_SINH:{
        long double ax=x<0.0L?-x:x;
        long double a=port109_x87_exp(ax);
        y=(a-1.0L/a)*0.5L;
        if(x<0.0L)y=-y;
        break; }
    case PORT109_COSH:{
        long double ax=x<0.0L?-x:x;
        long double a=port109_x87_exp(ax);
        y=(a+1.0L/a)*0.5L;
        break; }
    case PORT109_TANH:{
        long double ax=x<0.0L?-x:x;
        long double z=port109_x87_exp(-2.0L*ax);
        y=(1.0L-z)/(1.0L+z);
        if(x<0.0L)y=-y;
        break; }
    }
    if(degrees)y*=180.0L/port109_pi;
    return finish(f,y,out);
}
int port109_unary(enum port_m107_fmt f,enum port109_function op,
                  const uint16_t *in,uint16_t *out){
    return calculate(f,op,in,0,out);
}
int port109_unary_degrees(enum port_m107_fmt f,enum port109_function op,
                          const uint16_t *in,uint16_t *out){
    return calculate(f,op,in,1,out);
}
int port109_atan_ratio(enum port_m107_fmt f,const uint16_t *num,
                        const uint16_t *den,int degrees,uint16_t *out){
    if(!out||!num||!den||!fmt_valid(f)||(degrees!=0&&degrees!=1))
        return PORT109_BAD_ARG;
    long double x=0.0L,y=0.0L;
    int st=port109_decode(f,num,&x);if(st!=PORT109_OK)return st;
    st=port109_decode(f,den,&y);if(st!=PORT109_OK)return st;
    /* The source's ATAN2 entry point computes a quadrant-resolved X/Y.
     * It signals an invalid argument on (0,0), unlike ordinary atan2l. */
    if(x==0.0L && y==0.0L)return PORT109_DOMAIN;
    if(x==0.0L && y>0.0L){raw_zero(f,out);return PORT109_OK;}
    long double v=port109_x87_atan2(x,y);
    if(degrees)v*=180.0L/port109_pi;
    return finish(f,v,out);
}
