#include "vms_rtl_m113.h"
/* Bounded native subsets from VMS 3.7 RTL/SRC: independently word-decoded
 * VAX F/D values and packed-decimal/quadword operations, never VAX CPU ISA
 * execution. Every helper preflights its destination before writing. */
static int fmt_words(enum port_m107_fmt f){
    return f==PORT_M107_F?2:f==PORT_M107_D||f==PORT_M107_G?4:f==PORT_M107_H?8:0;
}
static uint64_t magnitude(int64_t n){
    uint64_t x=(uint64_t)n;return n<0?~x+1u:x;
}
static uint64_t div10(uint64_t n,unsigned *remainder){
    uint64_t q=0;unsigned r=0;
    for(int i=63;i>=0;i--){
        r=(r<<1)|(unsigned)((n>>(unsigned)i)&1u);
        if(r>=10u){r-=10u;q|=UINT64_C(1)<<(unsigned)i;}
    }
    if(remainder)*remainder=r;
    return q;
}
int port113_packed_decode(const uint8_t *in,size_t digits,int64_t *out){
    if(!in||!out||digits==0||digits>18u)return PORT113_BAD_ARG;
    /* Packed sign is the final low nibble, with a leading high pad nibble
     * for even decimal digit counts. VAX/Cobol signs A,C,E,F positive,
     * B,D negative. Reject non-BCD nibbles and nonzero leading pad. */
    size_t bytes=(digits+2u)/2u;
    unsigned sign=in[bytes-1u]&15u;
    if(sign<10u)return PORT113_BAD_ARG;
    unsigned neg=(sign==11u||sign==13u);
    uint64_t acc=0u;
    for(size_t i=0;i<digits;i++){
        size_t nibble=i+((digits&1u)==0u?1u:0u);
        unsigned d=(nibble&1u)?in[nibble/2u]&15u:in[nibble/2u]>>4;
        if(d>9u)return PORT113_BAD_ARG;
        acc=acc*10u+d;
    }
    if((digits&1u)==0u && (in[0]>>4)!=0u)return PORT113_BAD_ARG;
    *out=neg?-(int64_t)acc:(int64_t)acc;
    return PORT113_OK;
}
int port113_packed_encode(int64_t value,size_t digits,uint8_t *out,size_t cap){
    if(!out||!digits||digits>18u||cap<(digits+2u)/2u)return PORT113_BAD_ARG;
    uint8_t tmp[10]={0};
    uint64_t n=magnitude(value);
    for(size_t j=digits;j>0;j--){
        unsigned d=0;n=div10(n,&d);
        size_t nibble=(j-1u)+((digits&1u)==0u?1u:0u);
        if(nibble&1u)tmp[nibble/2u]|=(uint8_t)d;
        else tmp[nibble/2u]|=(uint8_t)(d<<4u);
    }
    if(n!=0u)return PORT113_OVERFLOW;
    size_t bytes=(digits+2u)/2u;
    tmp[bytes-1u]=(uint8_t)((tmp[bytes-1u]&0xf0u)|(value<0?0x0du:0x0cu));
    for(size_t i=0;i<bytes;i++)out[i]=tmp[i];
    return PORT113_OK;
}
static int mulscale(long double *v,int32_t scale){
    if(!v||scale<-30||scale>30)return PORT113_BAD_ARG;
    if(scale>=0){for(int i=0;i<scale;i++)*v*=10.0L;}
    else{for(int i=0;i>scale;i--)*v/=10.0L;}
    return __builtin_isfinite(*v)?PORT113_OK:PORT113_OVERFLOW;
}
static int to_i64(long double n,int round,int64_t *out){
    if(!out||!__builtin_isfinite(n))return PORT113_BAD_ARG;
    if(round)n+=n<0.0L?-0.5L:0.5L;
    if(n<(-9223372036854775808.0L)||n>=9223372036854775808.0L)
        return PORT113_OVERFLOW;
    *out=(int64_t)n;return PORT113_APPROX;
}
int port113_convert(enum port113_conv_kind kind,const void *src,size_t digits,
                    int32_t scale,void *dst,size_t cap,int rounded){
    if(!src||!dst||scale<-30||scale>30)return PORT113_BAD_ARG;
    if(digits>18u || (digits==0u&&(kind<=PORT113_PACK_QUAD)))return PORT113_BAD_ARG;
    long double v=0.0L;int64_t number=0;int status=PORT113_OK;
    enum port_m107_fmt fmt=PORT_M107_F;
    if(kind==PORT113_PACK_D||kind==PORT113_D_PACK||kind==PORT113_QUAD_D||kind==PORT113_D_QUAD)fmt=PORT_M107_D;
    if(kind==PORT113_PACK_F||kind==PORT113_PACK_D||kind==PORT113_PACK_QUAD){
        status=port113_packed_decode((const uint8_t *)src,digits,&number);
        if(status!=PORT113_OK)return status;
        v=(long double)number;
    }else if(kind==PORT113_QUAD_PACK||kind==PORT113_QUAD_F||kind==PORT113_QUAD_D){
        const uint32_t *w=(const uint32_t *)src;
        uint64_t q=((uint64_t)w[1]<<32)|w[0];
        number=(int64_t)q;v=(long double)number;
    }else if(kind==PORT113_F_PACK||kind==PORT113_D_PACK||kind==PORT113_F_QUAD||kind==PORT113_D_QUAD){
        status=port109_decode(fmt,(const uint16_t *)src,&v);
        if(status!=PORT109_OK)return PORT113_BAD_ARG;
    }else return PORT113_BAD_ARG;
    if(kind==PORT113_PACK_F||kind==PORT113_PACK_D){
        if(cap<(size_t)(fmt_words(fmt)*2))return PORT113_BAD_ARG;
        if(mulscale(&v,-scale)!=PORT113_OK)return PORT113_OVERFLOW;
        status=port109_encode(fmt,v,(uint16_t *)dst);
        return status==PORT109_OK?PORT113_APPROX:PORT113_OVERFLOW;
    }
    if(kind==PORT113_QUAD_F||kind==PORT113_QUAD_D){
        if(cap<(size_t)(fmt_words(fmt)*2))return PORT113_BAD_ARG;
        status=port109_encode(fmt,v,(uint16_t *)dst);
        return status==PORT109_OK?PORT113_APPROX:PORT113_OVERFLOW;
    }
    if(kind==PORT113_PACK_QUAD){
        if(cap<8u)return PORT113_BAD_ARG;
        if(mulscale(&v,-scale)!=PORT113_OK)return PORT113_OVERFLOW;
        status=to_i64(v,rounded,&number);if(status!=PORT113_APPROX)return status;
        uint64_t q=(uint64_t)number;
        uint32_t *out=dst;out[0]=(uint32_t)q;out[1]=(uint32_t)(q>>32);
        return PORT113_APPROX;
    }
    if(kind==PORT113_F_QUAD||kind==PORT113_D_QUAD){
        if(cap<8u)return PORT113_BAD_ARG;
        if(mulscale(&v,scale)!=PORT113_OK)return PORT113_OVERFLOW;
        status=to_i64(v,rounded,&number);if(status!=PORT113_APPROX)return status;
        uint64_t q=(uint64_t)number;
        uint32_t *out=dst;out[0]=(uint32_t)q;out[1]=(uint32_t)(q>>32);
        return PORT113_APPROX;
    }
    if(kind==PORT113_QUAD_PACK||kind==PORT113_F_PACK||kind==PORT113_D_PACK){
        if(mulscale(&v,scale)!=PORT113_OK)return PORT113_OVERFLOW;
        status=to_i64(v,rounded,&number);if(status!=PORT113_APPROX)return status;
        status=port113_packed_encode(number,digits,(uint8_t *)dst,cap);
        return status==PORT113_OK?PORT113_APPROX:status;
    }
    return PORT113_BAD_ARG;
}
int port113_addx(const uint32_t *a,const uint32_t *b,uint32_t *out,size_t n,int subtract){
    if(!a||!b||!out||!n||n>65535u||(subtract!=0&&subtract!=1))return PORT113_BAD_ARG;
    /* VAX ADDWC/SBWC carry or borrow, least-significant word first.
     * Native output may alias an input: every lane is read before write. */
    uint64_t carry=0u;
    for(size_t i=0;i<n;i++){
        uint32_t av=a[i],bv=b[i];
        if(subtract){uint64_t t=(uint64_t)av-(uint64_t)bv-carry;out[i]=(uint32_t)t;carry=(t>>63)&1u;}
        else{uint64_t t=(uint64_t)av+(uint64_t)bv+carry;out[i]=(uint32_t)t;carry=t>>32;}
    }
    return PORT113_OK;
}
static int matrix_shape(const struct port113_matrix *m,size_t *used,unsigned *words){
    if(!m||!m->words||!m->rows||!m->cols||!used||!words)return PORT113_BAD_ARG;
    int n=fmt_words(m->fmt);if(!n)return PORT113_BAD_ARG;
    size_t rows=m->rows,cols=m->cols;
    if(cols>SIZE_MAX/rows || rows*cols>SIZE_MAX/(unsigned)n ||
       rows*cols*(unsigned)n>m->words_capacity)return PORT113_BAD_ARG;
    *used=rows*cols*(unsigned)n;*words=(unsigned)n;return PORT113_OK;
}
int port113_matrix_fill(struct port113_matrix *m,int identity){
    size_t total=0;unsigned words=0;
    if(matrix_shape(m,&total,&words)!=PORT113_OK || (identity!=0&&identity!=1))return PORT113_BAD_ARG;
    uint16_t one[8]={0};
    if(identity && port109_encode(m->fmt,1.0L,one)!=PORT109_OK)return PORT113_UNSUPPORTED;
    for(size_t i=0;i<total;i++)m->words[i]=0u;
    if(identity){
        uint32_t k=m->rows<m->cols?m->rows:m->cols;
        for(uint32_t i=0;i<k;i++){
            size_t offset=((size_t)i*m->cols+i)*words;
            for(unsigned j=0;j<words;j++)m->words[offset+j]=one[j];
        }
    }
    return PORT113_OK;
}
int port113_matrix_redim(struct port113_matrix *m,uint32_t rows,uint32_t cols){
    if(!m||!rows||!cols)return PORT113_BAD_ARG;
    struct port113_matrix next=*m;next.rows=rows;next.cols=cols;
    size_t total=0;unsigned words=0;
    if(matrix_shape(&next,&total,&words)!=PORT113_OK)return PORT113_BAD_ARG;
    m->rows=rows;m->cols=cols;return PORT113_OK;
}
int port113_matrix_set(struct port113_matrix *m,uint32_t row,uint32_t col,const uint16_t *src){
    size_t total=0;unsigned words=0;
    if(!src||matrix_shape(m,&total,&words)!=PORT113_OK||row>=m->rows||col>=m->cols)
        return PORT113_BAD_ARG;
    size_t index=((size_t)row*m->cols+col)*words;
    for(unsigned j=0;j<words;j++)m->words[index+j]=src[j];
    return PORT113_OK;
}
int port113_matrix_null(struct port113_matrix *m){return port113_matrix_fill(m,0);}
int port113_matrix_initial(struct port113_matrix *m,const uint16_t *v){
    size_t total=0; unsigned words=0;
    if(!v || matrix_shape(m,&total,&words)!=PORT113_OK)return PORT113_BAD_ARG;
    for(size_t i=0;i<total;i++)m->words[i]=v[i%words];
    return PORT113_OK;
}

static int unsigned_div(uint64_t n,uint64_t d,uint64_t *q,uint64_t *r);
int port113_libday(uint64_t ticks,int32_t *days,uint32_t *centis){
    if(!days)return PORT113_BAD_ARG;
    /* Source epoch 17-NOV-1858; 10 million 100 ns ticks / second. */
    uint64_t day=0,within=0; if(unsigned_div(ticks,UINT64_C(864000000000),&day,&within)!=PORT113_OK)return PORT113_BAD_ARG;
    if(day>INT32_MAX)return PORT113_OVERFLOW;
    uint64_t cent=0,rem=0;
    if(centis && unsigned_div(within,UINT64_C(100000),&cent,&rem)!=PORT113_OK)return PORT113_BAD_ARG;
    *days=(int32_t)day;
    if(centis)*centis=(uint32_t)cent;
    return PORT113_OK;
}
int port113_libinitia(const struct port113_init_call *table,size_t n,int *out){
    if(!out||!table||n>1024u)return PORT113_BAD_ARG;
    for(size_t i=0;i<n;i++){
        if(!table[i].call)return PORT113_BAD_ARG;
    }
    for(size_t i=0;i<n;i++){
        int result=table[i].call(table[i].context);
        if(result!=PORT113_OK){*out=result;return PORT113_APPROX;}
    }
    *out=PORT113_OK;return PORT113_APPROX;
}
int port113_libtempla(const uint32_t *arg){return arg?PORT113_OK:PORT113_BAD_ARG;}
int port113_libastinp(uint32_t mode,unsigned ast_in_progress,int *out){
    if(!out||mode>3u||ast_in_progress>1u)return PORT113_BAD_ARG;
    *out=ast_in_progress!=0u;return PORT113_APPROX;
}
int port113_cobinvoke(int (*call)(void *,uint32_t),void *context,uint32_t arg,int *result){
    if(!call||!result)return PORT113_BAD_ARG;
    *result=call(context,arg);return PORT113_APPROX;
}
int port113_cobcnvout(int64_t v,char *out,size_t n,int asterisk_overflow){
    if(!out||!n||n>65535u)return PORT113_BAD_ARG;
    char tmp[22];size_t digits=0;uint64_t a=magnitude(v);
    do{unsigned d=0u;a=div10(a,&d);tmp[digits++]=(char)('0'+d);}while(a);
    if(v<0)tmp[digits++]='-';
    if(digits>n){for(size_t i=0;i<n;i++)out[i]=asterisk_overflow?'*':' ';
        return PORT113_OVERFLOW;}
    size_t i=0;while(i<n-digits)out[i++]=' ';
    while(digits)out[i++]=tmp[--digits];
    return PORT113_OK;
}
static int unsigned_div(uint64_t n,uint64_t d,uint64_t *q,uint64_t *r){
    if(!d||!q||!r)return PORT113_BAD_ARG;
    uint64_t quotient=0u,remainder=0u;
    for(int i=63;i>=0;i--){
        unsigned incoming=(unsigned)((n>>(unsigned)i)&1u);
        unsigned carry=(unsigned)(remainder>>63u);
        remainder=(remainder<<1u)|incoming;
        if(carry || remainder>=d){remainder-=d;quotient|=UINT64_C(1)<<(unsigned)i;}
    }
    *q=quotient;*r=remainder;return PORT113_OK;
}
int port113_cobdivq(int64_t n,int64_t d,int64_t *q,int64_t *r){
    if(!q||!r||d==0||(n==INT64_MIN&&d==-1))return PORT113_BAD_ARG;
    uint64_t uq=0u,ur=0u;
    if(unsigned_div(magnitude(n),magnitude(d),&uq,&ur)!=PORT113_OK)return PORT113_BAD_ARG;
    if((n<0)!=(d<0))*q=(uq==UINT64_C(0x8000000000000000))?INT64_MIN:-(int64_t)uq;
    else *q=(int64_t)uq;
    *r=n<0?-(int64_t)ur:(int64_t)ur;
    return PORT113_OK;
}
static int mul_checked(uint64_t a,uint64_t b,uint64_t lim,uint64_t *out){
    uint64_t value=0;
    while(b){
        if(b&1u){if(a>lim-value)return PORT113_OVERFLOW;value+=a;}
        b>>=1u;
        if(b){if(a>lim/2u)return PORT113_OVERFLOW;a<<=1u;}
    }
    *out=value;return PORT113_OK;
}
int port113_cobexpi(int64_t base,int32_t exponent,int64_t *out){
    if(!out)return PORT113_BAD_ARG;
    if(exponent<0)return PORT113_UNSUPPORTED; /* COB floating exponent cases pending */
    if(!base && !exponent)return PORT113_UNSUPPORTED;
    uint64_t b=magnitude(base),a=1u;
    uint64_t limit=(base<0&&(exponent&1u))?UINT64_C(0x8000000000000000):INT64_MAX;
    uint32_t e=(uint32_t)exponent;
    while(e){
        if(e&1u){uint64_t t=0;if(mul_checked(a,b,limit,&t)!=PORT113_OK)return PORT113_OVERFLOW;a=t;}
        e>>=1u;
        if(e){uint64_t t=0;if(mul_checked(b,b,limit,&t)!=PORT113_OK)return PORT113_OVERFLOW;b=t;}
    }
    *out=(base<0&&(exponent&1u))?((a==UINT64_C(0x8000000000000000))?INT64_MIN:-(int64_t)a):(int64_t)a;
    return PORT113_APPROX;
}
int port113_basedit(uint8_t *buffer,size_t n,int upcase,int brackets){
    if(!buffer||n>UINT16_MAX||(upcase!=0&&upcase!=1)||(brackets!=0&&brackets!=1))
        return PORT113_BAD_ARG;
    for(size_t i=0;i<n;i++){
        uint8_t x=buffer[i];
        if(upcase&&x>='a'&&x<='z')x=(uint8_t)(x-('a'-'A'));
        if(brackets&&x=='[')x='(';
        else if(brackets&&x==']')x=')';
        buffer[i]=x;
    }
    return PORT113_APPROX;
}
