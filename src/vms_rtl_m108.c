#include "vms_rtl_m108.h"
/* VAX memory-order 16-bit words: sign/exponent occupy the first word.
 * All shifts are on unsigned values and all arithmetic is bounded. */
static unsigned wc(enum port_m107_fmt f){return f==PORT_M107_F?2u:f==PORT_M107_H?8u:4u;}
static unsigned bias(enum port_m107_fmt f){return f==PORT_M107_G?1024u:f==PORT_M107_H?16384u:128u;}
static unsigned shift(enum port_m107_fmt f){return f==PORT_M107_G?4u:f==PORT_M107_H?0u:7u;}
static unsigned expn(enum port_m107_fmt f,const uint16_t *p){return (p[0]&0x7fffu)>>shift(f);}
static unsigned bits(enum port_m107_fmt f){return 1u+shift(f)+16u*(wc(f)-1u);}
static int validity(enum port_m107_fmt f,const uint16_t *a){
 int x=port_m107_float_valid(f,a);
 return x==PORT_M107_OK?PORT108_OK:x==PORT_M107_RESERVED_FLOAT?PORT108_RESERVED:PORT108_BAD_ARG;
}
static void zero(enum port_m107_fmt f,uint16_t *o){for(unsigned j=0;j<wc(f);j++)o[j]=0;}
static void cp(enum port_m107_fmt f,const uint16_t *a,uint16_t *o){for(unsigned j=0;j<wc(f);j++)o[j]=a[j];}
static unsigned sig(enum port_m107_fmt f,const uint16_t *a,unsigned i){
 if(i==0u)return 1u;
 if(i<=shift(f))return (a[0]>>(shift(f)-i))&1u;
 i-=shift(f)+1u;
 return (a[1u+i/16u]>>(15u-i%16u))&1u;
}
static void set_sig(enum port_m107_fmt f,uint16_t *a,unsigned i,unsigned bit){
 if(!i)return;
 unsigned w=0u,k=0u;
 if(i<=shift(f))k=shift(f)-i;
 else {i-=shift(f)+1u;w=1u+i/16u;k=15u-i%16u;}
 uint16_t mask=(uint16_t)(1u<<k);
 if(bit)a[w]|=mask;else a[w]&=(uint16_t)~mask;
}
/* All output words are committed only on success.  No hidden host IEEE conversion. */
int port108_float_transform(enum port_m107_fmt f,const uint16_t *a,uint16_t *out,enum port108_rounding how){
 if(!out||how>PORT108_ABS)return PORT108_BAD_ARG;
 int st=validity(f,a);if(st!=PORT108_OK)return st;
 uint16_t v[8]={0};cp(f,a,v);
 if(expn(f,a)==0){zero(f,out);return PORT108_OK;}
 if(how==PORT108_ABS){v[0]&=0x7fffu;cp(f,v,out);return PORT108_OK;}
 int e=(int)expn(f,a)-(int)bias(f),negative=!!(a[0]&0x8000u);
 if(e<=0){
  zero(f,v);
  if((how==PORT108_FLOOR&&negative)||(how==PORT108_NEAREST&&e==0)){
   v[0]=(uint16_t)(((bias(f)+1u)<<shift(f)) | (negative?0x8000u:0u));
  }
  cp(f,v,out);return PORT108_OK;
 }
 if(e>=(int)bits(f)){cp(f,v,out);return PORT108_OK;}
 int has_fraction=0;
 for(unsigned i=(unsigned)e;i<bits(f);i++)if(sig(f,a,i)){has_fraction=1;break;}
 unsigned round_up=0;
 if(how==PORT108_NEAREST)round_up=sig(f,a,(unsigned)e);
 else if(how==PORT108_FLOOR)round_up=negative&&has_fraction;
 for(unsigned i=(unsigned)e;i<bits(f);i++)set_sig(f,v,i,0);
 if(round_up){
  unsigned i=(unsigned)e-1u;
  for(;;){
   if(!i){
    unsigned newexp=expn(f,v)+1u;
    if(newexp >= (1u<<(15u-shift(f))))return PORT108_OVERFLOW;
    zero(f,v);v[0]=(uint16_t)((newexp<<shift(f))|(negative?0x8000u:0u));break;
   }
   if(!sig(f,v,i)){set_sig(f,v,i,1u);break;}
   set_sig(f,v,i,0u);i--;
  }
 }
 cp(f,v,out);return PORT108_OK;
}
int port108_float_integer(enum port_m107_fmt f,const uint16_t *a,int32_t *out){
 if(!out)return PORT108_BAD_ARG;
 int st=validity(f,a);if(st!=PORT108_OK)return st;
 if(!expn(f,a)){*out=0;return PORT108_OK;}
 int e=(int)expn(f,a)-(int)bias(f);
 if(e<=0||e>32||e>(int)bits(f))return PORT108_PARTIAL;
 uint32_t n=0;
 for(unsigned i=0;i<(unsigned)e;i++)n=(n<<1)|sig(f,a,i);
 for(unsigned i=(unsigned)e;i<bits(f);i++)if(sig(f,a,i))return PORT108_PARTIAL;
 int neg=!!(a[0]&0x8000u);
 if((!neg && n>INT32_MAX)||(neg && n>UINT32_C(2147483648)))return PORT108_PARTIAL;
 *out=neg?(int32_t)(0u-n):(int32_t)n;return PORT108_OK;
}
int port108_float_from_integer(enum port_m107_fmt f,int32_t x,uint16_t *out){
 if(!out || (f!=PORT_M107_F&&f!=PORT_M107_D&&f!=PORT_M107_G&&f!=PORT_M107_H))return PORT108_BAD_ARG;
 uint16_t v[8]={0};if(!x){zero(f,out);return PORT108_OK;}
 int sign=x<0;
 uint32_t n=sign?0u-(uint32_t)x:(uint32_t)x;
 unsigned e=0;for(uint32_t k=n;k;k>>=1)e++;
 if(e>bits(f))return PORT108_PARTIAL;
 unsigned field=bias(f)+e;
 if(field>=(1u<<(15u-shift(f))))return PORT108_OVERFLOW;
 v[0]=(uint16_t)((field<<shift(f))|(sign?0x8000u:0u));
 for(unsigned i=1;i<e;i++)set_sig(f,v,i,(n>>(e-1u-i))&1u);
 cp(f,v,out);return PORT108_OK;
}
int port108_float_mod(enum port_m107_fmt f,const uint16_t *a,const uint16_t *b,uint16_t *out){
 if(!out)return PORT108_BAD_ARG;
 int32_t x=0,y=0;int st=port108_float_integer(f,a,&x);if(st!=PORT108_OK)return st;
 st=port108_float_integer(f,b,&y);if(st!=PORT108_OK)return st;
 if(!y)return PORT108_DIVZERO;
 if(x==INT32_MIN&&y==-1)return PORT108_OVERFLOW;
 return port108_float_from_integer(f,x%y,out);
}
int port108_float_dim(enum port_m107_fmt f,const uint16_t *a,const uint16_t *b,uint16_t *out){
 if(!out)return PORT108_BAD_ARG;
 int32_t x=0,y=0;int st=port108_float_integer(f,a,&x);if(st!=PORT108_OK)return st;
 st=port108_float_integer(f,b,&y);if(st!=PORT108_OK)return st;
 int64_t v=(int64_t)x-(int64_t)y;
 if(v<=0){zero(f,out);return PORT108_OK;}
 if(v>INT32_MAX)return PORT108_PARTIAL;
 return port108_float_from_integer(f,(int32_t)v,out);
}
static uint32_t f24(const uint16_t v[2]){
 uint32_t s=UINT32_C(1)<<23;
 for(unsigned i=1;i<24;i++)s|=sig(PORT_M107_F,v,i)<<(23u-i);
 return s;
}
int port108_float_product(enum port_m107_fmt f,const uint16_t a[2],const uint16_t b[2],uint16_t *out){
 if(!out||(f!=PORT_M107_D&&f!=PORT_M107_G))return PORT108_BAD_ARG;
 int sa=validity(PORT_M107_F,a),sb=validity(PORT_M107_F,b);
 if(sa!=PORT108_OK)return sa;
 if(sb!=PORT108_OK)return sb;
 if(!expn(PORT_M107_F,a)||!expn(PORT_M107_F,b)){zero(f,out);return PORT108_OK;}
 uint64_t product=(uint64_t)f24(a)*(uint64_t)f24(b);
 unsigned hi=0;for(uint64_t t=product;t;t>>=1)hi++;
 int e=(int)expn(PORT_M107_F,a)-(int)bias(PORT_M107_F)+
       (int)expn(PORT_M107_F,b)-(int)bias(PORT_M107_F)+(int)hi-48;
 int field=e+(int)bias(f);
 if(field<=0||field>=(1<<(15u-shift(f))))return PORT108_OVERFLOW;
 uint16_t v[8]={0};v[0]=(uint16_t)(((unsigned)field<<shift(f))|((a[0]^b[0])&0x8000u));
 for(unsigned i=1;i<bits(f)&&i<hi;i++)set_sig(f,v,i,(unsigned)((product>>(hi-1u-i))&1u));
 cp(f,v,out);return PORT108_OK;
}
int port108_float_cabs(enum port_m107_fmt f,const uint16_t *pair,uint16_t *out){
 if(!pair||!out||f==PORT_M107_H)return PORT108_BAD_ARG;
 int32_t a=0,b=0;int st=port108_float_integer(f,pair,&a);if(st!=PORT108_OK)return st;
 st=port108_float_integer(f,pair+wc(f),&b);if(st!=PORT108_OK)return st;
 uint64_t square=(uint64_t)((int64_t)a*a)+(uint64_t)((int64_t)b*b);
 /* Exact integer result subset; do not pretend that host hypot has VAX rounding. */
 uint64_t lo=0,hi=INT32_MAX;
 while(lo<hi){uint64_t mid=(lo+hi+1u)/2u;
  if(mid*mid<=square)lo=mid;else hi=mid-1u;
 }
 if(lo*lo!=square)return PORT108_PARTIAL;
 return port108_float_from_integer(f,(int32_t)lo,out);
}
int port108_integer_abs16(int16_t a,int16_t *out){if(!out)return PORT108_BAD_ARG;*out=a<0?(int16_t)(uint16_t)(0u-(uint16_t)a):a;return PORT108_OK;}
int port108_integer_abs32(int32_t a,int32_t *out){if(!out)return PORT108_BAD_ARG;*out=a<0?(int32_t)(0u-(uint32_t)a):a;return PORT108_OK;}
int port108_integer_dim16(int16_t a,int16_t b,int16_t *out){if(!out)return PORT108_BAD_ARG;int16_t x=(int16_t)(uint16_t)((uint16_t)a-(uint16_t)b);*out=x>0?x:0;return PORT108_OK;}
int port108_integer_dim32(int32_t a,int32_t b,int32_t *out){if(!out)return PORT108_BAD_ARG;int32_t x=(int32_t)((uint32_t)a-(uint32_t)b);*out=x>0?x:0;return PORT108_OK;}
int port108_integer_mod16(int16_t a,int16_t b,int16_t *out){if(!out)return PORT108_BAD_ARG;if(!b)return PORT108_DIVZERO;*out=(int16_t)((int32_t)a%b);return PORT108_OK;}
int port108_integer_mod32(int32_t a,int32_t b,int32_t *out){if(!out)return PORT108_BAD_ARG;if(!b)return PORT108_DIVZERO;if(a==INT32_MIN&&b==-1)return PORT108_OVERFLOW;*out=a%b;return PORT108_OK;}
/* Ten original MTHBITOPS entry points; operation 0=AND,1=OR,2=XOR,3=NOT,4=shift. */
int port108_math_bit16(unsigned op,uint16_t a,uint16_t b,uint16_t *out){
 if(!out||op>4)return PORT108_BAD_ARG;
 uint16_t r=0;
 if(op==0)r=a&b;else if(op==1)r=a|b;else if(op==2)r=a^b;else if(op==3)r=(uint16_t)~a;
 else {int16_t shift=(int16_t)b;r=shift>=0?(shift>=16?0u:(uint16_t)(a<<shift)):(shift<=-16?0u:(uint16_t)(a>>(-shift)));}
 *out=r;return PORT108_OK;
}
int port108_math_bit32(unsigned op,uint32_t a,uint32_t b,uint32_t *out){
 if(!out||op>4)return PORT108_BAD_ARG;
 uint32_t r=0;
 if(op==0)r=a&b;else if(op==1)r=a|b;else if(op==2)r=a^b;else if(op==3)r=~a;
 else{int32_t shift=(int32_t)b;r=shift>=0?(shift>=32?0u:a<<shift):(shift<=-32?0u:a>>(-shift));}
 *out=r;return PORT108_OK;
}
static int fraction24(uint32_t value,unsigned width,uint16_t out[2]){
 if(!value){out[0]=out[1]=0;return PORT108_OK;}
 unsigned hi=0;for(uint32_t t=value;t;t>>=1)hi++;
 int e=(int)hi-(int)width;
 uint16_t v[2]={0};
 int field=128+e;if(field<=0||field>=256)return PORT108_OVERFLOW;
 v[0]=(uint16_t)((unsigned)field<<7);
 for(unsigned i=1;i<24;i++){unsigned bit=hi>i ? ((value>>(hi-1u-i))&1u):0u;set_sig(PORT_M107_F,v,i,bit);}
 out[0]=v[0];out[1]=v[1];return PORT108_OK;
}
int port108_random_mth(uint32_t *seed,uint16_t out[2]){
 if(!seed||!out)return PORT108_BAD_ARG;
 *seed *= UINT32_C(69069);
 return fraction24(*seed>>8,24u,out);
}
int port108_random_fortran(uint16_t *high,uint16_t *low,uint16_t out[2]){
 if(!high||!low||!out)return PORT108_BAD_ARG;
 uint32_t seed=((uint32_t)*high<<16)|*low;
 if(!seed)seed=3u;else seed=seed*UINT32_C(65539)&UINT32_C(0x7fffffff);
 *high=(uint16_t)(seed>>16);*low=(uint16_t)seed;
 /* F-format has 24 significant bits; the 31-bit source seed is truncated
  * after bit 24. Rounding of the original CVTLF remains unresolved. */
 return fraction24(seed,31u,out);
}
static uint32_t load32(const uint8_t *p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static void store32(uint8_t *p,uint32_t x){for(unsigned j=0;j<4;j++)p[j]=(uint8_t)(x>>(8u*j));}
static uint8_t *loc(const struct port108_queue *q,uint32_t address){
 if(!q||!q->mem||(q->base&3u)||(address&3u)||address<q->base)return 0;
 uint64_t off=(uint64_t)address-q->base;
 if(off+8u>q->bytes)return 0;
 return q->mem+(size_t)off;
}
int port108_queue_init(struct port108_queue *q){
 uint8_t *h=loc(q,q?q->head:0);if(!h)return PORT108_BAD_ARG;
 store32(h,q->head);store32(h+4,q->head);return PORT108_OK;
}
int port108_queue_insert(struct port108_queue *q,uint32_t entry,int at_tail){
 if(!q || ((uint64_t)entry >= q->head && (uint64_t)entry < (uint64_t)q->head+8u))return PORT108_BAD_ARG;
 uint8_t *head=loc(q,q?q->head:0),*p=loc(q,entry);
 if(!head||!p)return PORT108_BAD_ARG;
 uint32_t first=load32(head),last=load32(head+4);
 uint8_t *f=loc(q,first),*l=loc(q,last);
 if(!f||!l||(first==q->head&&load32(f+4)!=q->head))return PORT108_BAD_ARG;
 if((first==q->head)!=(last==q->head))return PORT108_BAD_ARG;
 if(load32(p)||load32(p+4))return PORT108_BUSY;
 uint8_t *n=at_tail?head:f,*prev=at_tail?l:head;
 uint32_t na=at_tail?q->head:first,pa=at_tail?last:q->head;
 if(load32(prev)!=na||load32(n+4)!=pa)return PORT108_BAD_ARG;
 store32(p,na);store32(p+4,pa);
 store32(prev,entry);store32(n+4,entry);
 return first==q->head?PORT108_ONE:PORT108_OK;
}
int port108_queue_remove(struct port108_queue *q,uint32_t *removed,int at_tail){
 uint8_t *head=loc(q,q?q->head:0);if(!head||!removed)return PORT108_BAD_ARG;
 uint32_t entry=load32(head+(at_tail?4:0));
 if(entry==q->head){*removed=0;return PORT108_EMPTY;}
 uint8_t *p=loc(q,entry);if(!p)return PORT108_BAD_ARG;
 uint32_t next=load32(p),prev=load32(p+4);
 uint8_t *n=loc(q,next),*pr=loc(q,prev);
 if(!n||!pr||load32(pr)!=entry||load32(n+4)!=entry)return PORT108_BAD_ARG;
 store32(pr,next);store32(n+4,prev);store32(p,0);store32(p+4,0);
 *removed=entry;return load32(head)==q->head?PORT108_ONE:PORT108_OK;
}
int port108_movq(const uint8_t *s,uint8_t *d){if(!s||!d)return PORT108_BAD_ARG;uint8_t t[8];for(unsigned i=0;i<8;i++)t[i]=s[i];for(unsigned i=0;i<8;i++)d[i]=t[i];return PORT108_OK;}
int port108_movtc(const uint8_t *s,size_t ns,const uint8_t tab[256],uint8_t fill,uint8_t *d,size_t nd){
 if(!tab||(!s&&ns)||(!d&&nd))return PORT108_BAD_ARG;
 /* Source and dest overlap is not supported by this wrapper (MOVTC register
  * directions and overlap behavior still require a translated VAX frame). */
 if(s&&d){
  uintptr_t a=(uintptr_t)s,b=(uintptr_t)d;
  if((a<=b && b-a<ns)||(b<a && a-b<nd))return PORT108_PARTIAL;
 }
 for(size_t i=0;i<nd;i++)d[i]=i<ns?tab[s[i]]:fill;
 return PORT108_OK;
}
int port108_movtuc(const uint8_t *s,size_t ns,const uint8_t tab[256],uint8_t esc,uint8_t fill,uint8_t *d,size_t nd,size_t *index){
 if(!tab||(!s&&ns)||(!d&&nd)||!index)return PORT108_BAD_ARG;
 if(s&&d){
  uintptr_t a=(uintptr_t)s,b=(uintptr_t)d;
  if((a<=b && b-a<ns)||(b<a && a-b<nd))return PORT108_PARTIAL;
 }
 size_t i=0;while(i<ns&&i<nd&&tab[s[i]]!=esc){d[i]=tab[s[i]];i++;}
 *index=(i<ns&&i<nd&&tab[s[i]]==esc)?i+1u:0u;
 for(size_t j=i;j<nd;j++)d[j]=fill;
 return PORT108_OK;
}
uint32_t port108_crc_reference(uint32_t poly,uint32_t crc,const uint8_t *data,size_t n){
 if(!data&&n)return crc;
 for(size_t i=0;i<n;i++){crc^=data[i];for(unsigned k=0;k<8;k++)crc=(crc>>1)^((crc&1u)?poly:0u);}
 return crc;
}
int port108_emul(int32_t a,int32_t b,int32_t add,int64_t *out){if(!out)return PORT108_BAD_ARG;*out=(int64_t)a*b+add;return PORT108_OK;}
int port108_ediv(int32_t d,int64_t v,int32_t *q,int32_t *r){
 if(!q||!r)return PORT108_BAD_ARG;
 if(!d)return PORT108_DIVZERO;
 if(d==-1 && v==INT64_MIN)return PORT108_OVERFLOW;
 int64_t quot=v/d;if(quot<INT32_MIN||quot>INT32_MAX)return PORT108_OVERFLOW;
 *q=(int32_t)quot;*r=(int32_t)(v%d);return PORT108_OK;
}
int port108_get_opcode(const uint8_t *p,size_t n,uint16_t *out){
 if(!p||!out||!n)return PORT108_BAD_ARG;
 if(p[0]>=0xfdu){if(n<2)return PORT108_BAD_ARG;*out=(uint16_t)p[0]|(uint16_t)p[1]<<8;}
 else *out=p[0];
 return PORT108_OK;
}
