#include "vms_rtl_m111.h"
#include "vms_rtl_m109_x87.h"
/* Native source-derived subsets. For source-only DEC references, no original
 * DEC source code or machine image is redistributed. This file is explicitly
 * NOT linked into SYS.EXE, so live guest validation is outstanding. */
static unsigned nwords(enum port_m107_fmt fmt){
    return fmt==PORT_M107_F?2u:(fmt==PORT_M107_D||fmt==PORT_M107_G)?4u:fmt==PORT_M107_H?8u:0u;
}
static void words_copy(uint16_t *dst,const uint16_t *src,unsigned n){
    for(unsigned i=0;i<n;i++)dst[i]=src[i];
}
static void bytes_copy(uint8_t *dst,const uint8_t *src,size_t n){
    for(size_t i=0;i<n;i++)dst[i]=src[i];
}
static int good(long double v){return __builtin_isfinite(v);}
static long double absolute(long double v){return v<0?-v:v;}
static int read_pair(enum port_m107_fmt f,const uint16_t *p,long double *a,long double *b){
    int s=port109_decode(f,p,a);if(s!=PORT109_OK)return s;
    return port109_decode(f,p+nwords(f),b);
}
static int write_scalar(enum port_m107_fmt f,long double v,uint16_t *out){
    uint16_t local[8]={0};
    if(!good(v))return PORT109_RANGE;
    int s=port109_encode(f,v,local);if(s!=PORT109_OK)return s;
    words_copy(out,local,nwords(f));return PORT109_APPROX;
}
static int write_pair(enum port_m107_fmt f,long double a,long double b,uint16_t *out){
    uint16_t local[16]={0};unsigned n=nwords(f);
    if(!good(a)||!good(b))return PORT109_RANGE;
    int s=port109_encode(f,a,local);if(s!=PORT109_OK)return s;
    s=port109_encode(f,b,local+n);if(s!=PORT109_OK)return s;
    words_copy(out,local,2*n);return PORT109_APPROX;
}
static int leap(unsigned y){return y%4==0&&(y%100!=0||y%400==0);}
int port111_clock_valid(const struct port111_clock *t){
    static const unsigned month_days[12]={31,28,31,30,31,30,31,31,30,31,30,31};
    if(!t||t->year<1||t->year>9999||t->month<1||t->month>12||t->hour>23||t->minute>59||t->second>59||t->centisecond>99)return 0;
    unsigned days=month_days[t->month-1]+(t->month==2&&leap(t->year));
    return t->day>0&&t->day<=days;
}
static void two(unsigned value,char *dest){dest[0]=(char)('0'+value/10);dest[1]=(char)('0'+value%10);}
int port111_date(const struct port111_clock *t,char *out,size_t capacity,int is_time){
    static const char months[12][3]={{'J','A','N'},{'F','E','B'},{'M','A','R'},{'A','P','R'},
        {'M','A','Y'},{'J','U','N'},{'J','U','L'},{'A','U','G'},
        {'S','E','P'},{'O','C','T'},{'N','O','V'},{'D','E','C'}};
    size_t required=is_time?8u:9u;
    if(!out||capacity<required||!port111_clock_valid(t))return PORT109_BAD_ARG;
    char local[9];
    if(is_time){two(t->hour,local);local[2]=':';two(t->minute,local+3);
        local[5]=':';two(t->second,local+6);}
    else{two(t->day,local);local[2]='-';for(unsigned i=0;i<3;i++)local[3+i]=months[t->month-1][i];
        local[6]='-';two(t->year%100,local+7);}
    bytes_copy((uint8_t*)out,(const uint8_t*)local,required);return PORT109_OK;
}
int port111_date_descriptor(const struct port111_clock *t,char *out,size_t cap,int is_time){
    if(!out||cap>65535u||!port111_clock_valid(t))return PORT109_BAD_ARG;
    char temp[9]={0};int s=port111_date(t,temp,sizeof temp,is_time);if(s!=PORT109_OK)return s;
    size_t n=is_time?8u:9u;
    for(size_t i=0;i<cap;i++)out[i]=i<n?temp[i]:' ';
    return PORT109_OK;
}
int port111_mdy(const struct port111_clock *t,void *month,void *day,void *year,unsigned bits){
    if(!port111_clock_valid(t)||!month||!day||!year||(bits!=16&&bits!=32))return PORT109_BAD_ARG;
    unsigned yy=t->year%100u;
    if(bits==16){*(int16_t*)month=(int16_t)t->month;*(int16_t*)day=(int16_t)t->day;*(int16_t*)year=(int16_t)yy;}
    else{*(int32_t*)month=(int32_t)t->month;*(int32_t*)day=(int32_t)t->day;*(int32_t*)year=(int32_t)yy;}
    return PORT109_OK;
}
int port111_seconds(const struct port111_clock *t,const uint16_t *start,uint16_t *out){
    if(!port111_clock_valid(t)||!start||!out)return PORT109_BAD_ARG;
    long double begin=0;
    int s=port109_decode(PORT_M107_F,start,&begin);if(s!=PORT109_OK)return s;
    long double now=(long double)(t->hour*3600u+t->minute*60u+t->second)+((long double)t->centisecond)/100.0L;
    long double elapsed=now-begin;
    if(elapsed<0.0L)elapsed+=86400.0L; /* VMS FOR$SECNDS rollover */
    return write_scalar(PORT_M107_F,elapsed,out);
}
int port111_complex_mul(enum port_m107_fmt f,const uint16_t *a,const uint16_t *b,uint16_t *out){
    if(!nwords(f)||!a||!b||!out)return PORT109_BAD_ARG;
    long double ar,ai,br,bi;
    int s=read_pair(f,a,&ar,&ai);if(s!=PORT109_OK)return s;
    s=read_pair(f,b,&br,&bi);if(s!=PORT109_OK)return s;
    return write_pair(f,ar*br-ai*bi,ar*bi+ai*br,out);
}
int port111_complex_div(enum port_m107_fmt f,const uint16_t *a,const uint16_t *b,uint16_t *out){
    if(!nwords(f)||!a||!b||!out)return PORT109_BAD_ARG;
    long double ar,ai,br,bi;
    int s=read_pair(f,a,&ar,&ai);if(s!=PORT109_OK)return s;
    s=read_pair(f,b,&br,&bi);if(s!=PORT109_OK)return s;
    long double scale=absolute(br)>absolute(bi)?absolute(br):absolute(bi);
    if(scale==0.0L)return PORT109_DOMAIN;
    /* Native scale matches intent of the original exponent scaling: avoid
       overflow and spurious divide faults from large complex denominators. */
    long double cr=br/scale,ci=bi/scale,den=cr*cr+ci*ci;
    return write_pair(f,(ar*cr+ai*ci)/(den*scale),(ai*cr-ar*ci)/(den*scale),out);
}
int port111_complex_pow(enum port_m107_fmt f,const uint16_t *base,const uint16_t *exponent,uint16_t *out){
    if(!nwords(f)||!base||!exponent||!out)return PORT109_BAD_ARG;
    long double ar,ai,br,bi;
    int s=read_pair(f,base,&ar,&ai);if(s!=PORT109_OK)return s;
    s=read_pair(f,exponent,&br,&bi);if(s!=PORT109_OK)return s;
    if(ar==0.0L&&ai==0.0L){if(br<=0.0L||bi!=0.0L)return PORT109_DOMAIN;return write_pair(f,0,0,out);}
    long double radius=port109_x87_sqrt(ar*ar+ai*ai);
    long double x=port109_x87_ln(radius),y=port109_x87_atan2(ai,ar);
    long double er=br*x-bi*y,ei=bi*x+br*y;
    if(absolute(ei)>1.0e6L)return PORT109_PARTIAL;
    long double mag=port109_x87_exp(er);
    return write_pair(f,mag*port109_x87_cos(ei),mag*port109_x87_sin(ei),out);
}
int port111_complex_intpow(enum port_m107_fmt f,const uint16_t *base,int32_t exp,uint16_t *out){
    if(!nwords(f)||!base||!out)return PORT109_BAD_ARG;
    long double ar,ai;int s=read_pair(f,base,&ar,&ai);if(s!=PORT109_OK)return s;
    if(ar==0.0L&&ai==0.0L&&exp<=0)return PORT109_DOMAIN;
    long double rr=1,ri=0;long double xr=ar,xi=ai;
    uint32_t n=exp<0?(uint32_t)(-(int64_t)exp):(uint32_t)exp;
    while(n){if(n&1u){long double nr=rr*xr-ri*xi,ni=rr*xi+ri*xr;rr=nr;ri=ni;}
        n>>=1;if(n){long double nr=xr*xr-xi*xi,ni=2*xr*xi;xr=nr;xi=ni;}
        if(!good(rr)||!good(ri)||!good(xr)||!good(xi))return PORT109_RANGE;}
    if(exp<0){long double den=rr*rr+ri*ri;if(den==0.0L)return PORT109_RANGE;
        long double re=rr/den;ri=-ri/den;rr=re;}
    return write_pair(f,rr,ri,out);
}
int port111_real_pow(enum port_m107_fmt f,const uint16_t *base,const uint16_t *exponent,uint16_t *out,int basic){
    if(!nwords(f)||!base||!exponent||!out)return PORT109_BAD_ARG;
    long double b,e;int s=port109_decode(f,base,&b);if(s!=PORT109_OK)return s;
    s=port109_decode(f,exponent,&e);if(s!=PORT109_OK)return s;
    if(b==0.0L){if(e<=0.0L)return PORT109_DOMAIN;return write_scalar(f,0.0L,out);}
    if(e==0.0L)return write_scalar(f,1.0L,out);
    int negative=(b<0);
    if(negative&&!basic)return PORT109_DOMAIN; /* OTS negative real base rejects */
    if(negative&&(absolute(e)>2147483647.0L||e!=(long double)(int32_t)e))return PORT109_DOMAIN;
    long double mag=port109_x87_exp(e*port109_x87_ln(absolute(b)));
    if(negative&&((int32_t)e&1))mag=-mag;
    return write_scalar(f,mag,out);
}
int port111_real_intpow(enum port_m107_fmt f,const uint16_t *base,int32_t exp,uint16_t *out,int basic){
    if(!nwords(f)||!base||!out)return PORT109_BAD_ARG;
    long double b;int s=port109_decode(f,base,&b);if(s!=PORT109_OK)return s;
    if(b==0.0L&&exp<=0)return PORT109_DOMAIN;
    if(b<0.0L&&!basic)return PORT109_DOMAIN;
    long double prod=1,x=b;uint32_t e=exp<0?(uint32_t)(-(int64_t)exp):(uint32_t)exp;
    while(e){if(e&1u)prod*=x;e>>=1;if(e)x*=x;
        if(!good(prod)||!good(x))return PORT109_RANGE;}
    if(exp<0)prod=1.0L/prod;
    return write_scalar(f,prod,out);
}
int port111_integer_pow(int64_t base,int64_t exponent,unsigned bits,int64_t *out){
    if(!out||(bits!=16&&bits!=32))return PORT109_BAD_ARG;
    int64_t limit=bits==16?32767:2147483647;
    int64_t lower=-limit-1;
    if(base<lower||base>limit)return PORT109_BAD_ARG;
    if(exponent<=0){
        if(base==0)return PORT109_DOMAIN;
        *out=exponent==0?1:(base==1?1:(base==-1?((uint64_t)exponent&1u)?-1:1:0));
        return PORT109_OK;
    }
    int64_t acc=1,x=base;uint64_t e=(uint64_t)exponent;
    while(e){if(e&1u){acc*=x;if(acc>limit||acc<lower)return PORT109_RANGE;}
        e>>=1;if(e){x*=x;if(x>limit||x<lower)return PORT109_RANGE;}}
    *out=acc;return PORT109_OK;
}
int port111_ulogpow(uint32_t base,uint32_t exponent,uint32_t *out){
    if(!out)return PORT109_BAD_ARG;
    if(base==0&&exponent==0)return PORT109_DOMAIN;
    uint32_t acc=1,x=base;
    while(exponent){if(exponent&1u)acc*=x;exponent>>=1;if(exponent)x*=x;}
    *out=acc;return PORT109_OK; /* Defined unsigned modulo 2^32 */
}
void port111_lun_init(struct port111_lun *p){if(p)for(unsigned i=0;i<4;i++)p->bits[i]=0;}
int port111_lun_get(struct port111_lun *p,unsigned *unit){
    if(!p||!unit)return PORT109_BAD_ARG;
    for(unsigned i=0;i<100;i++){uint32_t mask=1u<<(i%32u);uint32_t *slot=&p->bits[i/32u];
        if(!(*slot&mask)){*slot|=mask;*unit=i;return PORT109_OK;}}
    return PORT109_PARTIAL;
}
int port111_lun_free(struct port111_lun *p,unsigned unit){
    if(!p||unit>=100)return PORT109_BAD_ARG;
    uint32_t mask=1u<<(unit%32),*slot=&p->bits[unit/32];
    if(!(*slot&mask))return PORT109_PARTIAL;
    *slot&=~mask;return PORT109_OK;
}
int port111_lun_take(struct port111_lun *p,unsigned unit){
    if(!p||unit>=100)return PORT109_BAD_ARG;
    uint32_t mask=1u<<(unit%32),*slot=&p->bits[unit/32];
    if(*slot&mask)return PORT109_PARTIAL;
    *slot|=mask;return PORT109_OK;
}
static int put_int(void *out,unsigned bits,int32_t v){
    if(!out||(bits!=8&&bits!=16&&bits!=32))return PORT109_BAD_ARG;
    if(bits==8){if(v< -128||v>127)return PORT109_RANGE;*(int8_t*)out=(int8_t)v;}
    else if(bits==16){if(v< -32768||v>32767)return PORT109_RANGE;*(int16_t*)out=(int16_t)v;}
    else *(int32_t*)out=v;
    return PORT109_OK;
}
int port111_parse_logical(const char *text,size_t n,unsigned bits,void *out){
    if(!text||!out||n>65535)return PORT109_BAD_ARG;
    size_t i=0;while(i<n&&text[i]==' ')i++;
    int v=0;
    if(i<n){if(text[i]=='.')i++;
        if(i>=n)return PORT109_BAD_ARG;
        char c=text[i];if(c=='T'||c=='t')v=-1;else if(c!='F'&&c!='f')return PORT109_BAD_ARG;}
    return put_int(out,bits,v);
}
int port111_parse_integer(const char *text,size_t n,unsigned bits,int tabs,void *out){
    if(!text||!out||n>65535||(tabs!=0&&tabs!=1))return PORT109_BAD_ARG;
    size_t i=0;
    while(i<n&&(text[i]==' '||(tabs&&text[i]=='\t')))i++;
    int negative=0;if(i<n&&(text[i]=='-'||text[i]=='+'))negative=(text[i++]=='-');
    int64_t value=0;size_t digits=0;
    for(;i<n;i++){
        char c=text[i];if(c==' ')c='0';
        if(c<'0'||c>'9')return PORT109_BAD_ARG;
        value=value*10+(c-'0');digits++;
        if(value>2147483648LL)return PORT109_RANGE;
    }
    /* OTS$CVT_TI_L's SKPC branch treats an all-blank field as zero.
       An isolated sign similarly reaches DONE with an empty accumulator. */
    if(!digits)return put_int(out,bits,0);
    if(negative)value=-value;
    if(value<(-2147483647LL-1)||value>2147483647LL)return PORT109_RANGE;
    return put_int(out,bits,(int32_t)value);
}
int port111_move(const void *src,void *dst,size_t n){
    if(n>0&&(!src||!dst))return PORT109_BAD_ARG;
    if(n>0xffffffffu)return PORT109_BAD_ARG;
    if(n==0||src==dst)return PORT109_OK;
    const uint8_t *from=src;uint8_t *to=dst;
    /* Source MOVE3_R5 handles multi-segment overlap. Never rely on UB from
       comparisons of pointers to unrelated objects; use integer addresses. */
    uintptr_t a=(uintptr_t)from,b=(uintptr_t)to;
    if(b>a&&b-a<n){for(size_t i=n;i!=0;i--)to[i-1]=from[i-1];}
    else{for(size_t i=0;i<n;i++)to[i]=from[i];}
    return PORT109_OK;
}
int port111_d_arith(const uint16_t *a,const uint16_t *b,uint16_t *out,int divide){
    if(!a||!b||!out||(divide!=0&&divide!=1))return PORT109_BAD_ARG;
    long double x,y;int s=port109_decode(PORT_M107_D,a,&x);if(s!=PORT109_OK)return s;
    s=port109_decode(PORT_M107_D,b,&y);if(s!=PORT109_OK)return s;
    if(divide&&y==0)return PORT109_DOMAIN;
    return write_scalar(PORT_M107_D,divide?x/y:x*y,out);
}
int port111_copy_float(enum port_m107_fmt fmt,const uint16_t *src,uint16_t *dst){
    if(!nwords(fmt)||!src||!dst)return PORT109_BAD_ARG;
    /* MOVF/MOVD can trap reserved operands, unlike raw MOVL/MOVQ. */
    long double decoded;int s=port109_decode(fmt,src,&decoded);
    if(s!=PORT109_OK)return s;
    words_copy(dst,src,nwords(fmt));return PORT109_OK;
}
int port111_det_store(struct port111_det *d,const uint16_t *v){
    if(!d||!v)return PORT109_BAD_ARG;
    long double decoded;int s=port109_decode(PORT_M107_D,v,&decoded);if(s!=PORT109_OK)return s;
    words_copy(d->d,v,4);return PORT109_OK;
}
int port111_det_get(const struct port111_det *d,enum port_m107_fmt fmt,uint16_t *out){
    if(!d||!out||(fmt!=PORT_M107_F&&fmt!=PORT_M107_D))return PORT109_BAD_ARG;
    long double x;int s=port109_decode(PORT_M107_D,d->d,&x);if(s!=PORT109_OK)return s;
    if(fmt==PORT_M107_D){words_copy(out,d->d,4);return PORT109_OK;}
    return write_scalar(PORT_M107_F,x,out);
}
int port111_xlate(const uint8_t *src,size_t n,const uint8_t *table,size_t table_n,
                  uint8_t *dst,size_t cap,size_t *written){
    if(!src||!table||!dst||!written||table_n>256||cap<n)return PORT109_BAD_ARG;
    /* BAS$XLATE MOVTUC: zero means deletion, not an output NUL. */
    size_t actual=0;
    for(size_t i=0;i<n;i++){
        uint8_t c=src[i],replacement=c<table_n?table[c]:0;
        if(replacement!=0)dst[actual++]=replacement;
    }
    *written=actual;
    return PORT109_APPROX; /* Caller still owns the complete BASIC descriptor. */
}
int port111_mulq(const uint32_t a[2],const uint32_t b[2],uint32_t out[2]){
    if(!a||!b||!out)return PORT109_BAD_ARG;
    uint64_t lo=(uint64_t)a[0]*(uint64_t)b[0];
    uint32_t hi=(uint32_t)(lo>>32)+(uint32_t)(a[0]*b[1])+(uint32_t)(a[1]*b[0]);
    uint32_t outlow=(uint32_t)lo;
    out[0]=outlow;out[1]=hi;return PORT109_OK; /* unsigned modulo 2^64 */
}
