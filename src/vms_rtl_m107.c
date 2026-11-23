#include "vms_rtl_m107.h"
/* Indexed VAX significance; the low-addressed 16-bit word contains the
 * exponent, sign, and the most significant fraction bits. */
static unsigned word_count(enum port_m107_fmt f){ return f==PORT_M107_F?2u:f==PORT_M107_H?8u:4u; }
static unsigned bias(enum port_m107_fmt f){return f==PORT_M107_G?1024u:f==PORT_M107_H?16384u:128u;}
static unsigned shift(enum port_m107_fmt f){return f==PORT_M107_G?4u:f==PORT_M107_H?0u:7u;}
static unsigned exponent(enum port_m107_fmt f,const uint16_t *p){
 return (p[0] & UINT16_C(0x7fff)) >> shift(f);
}
int port_m107_float_valid(enum port_m107_fmt f,const uint16_t *p){
 if(!p || (f!=PORT_M107_F&&f!=PORT_M107_D&&f!=PORT_M107_G&&f!=PORT_M107_H))return PORT_M107_BAD_ARG;
 return !exponent(f,p) && (p[0]&UINT16_C(0x8000))?PORT_M107_RESERVED_FLOAT:PORT_M107_OK;
}
static int is_zero(enum port_m107_fmt f,const uint16_t *p){return exponent(f,p)==0u;}
int port_m107_float_cmp(enum port_m107_fmt f,const uint16_t *a,const uint16_t *b,int *relation){
 if(!relation)return PORT_M107_BAD_ARG;
 int s=port_m107_float_valid(f,a),t=port_m107_float_valid(f,b);
 if(s!=PORT_M107_OK)return s;
 if(t!=PORT_M107_OK)return t;
 unsigned as=(a[0]>>15)&1u,bs=(b[0]>>15)&1u;
 int m=0;
 if(is_zero(f,a)&&is_zero(f,b))m=0;
 else if(is_zero(f,a))m=bs?1:-1;
 else if(is_zero(f,b))m=as?-1:1;
 else if(as!=bs)m=as?-1:1;
 else{
  unsigned n=word_count(f);
  for(unsigned i=0;i<n;i++){
   uint16_t aw=i?a[i]:a[i]&UINT16_C(0x7fff);
   uint16_t bw=i?b[i]:b[i]&UINT16_C(0x7fff);
   if(aw!=bw){m=aw>bw?1:-1;break;}
  }
  if(as)m=-m;
 }
 *relation=m;return PORT_M107_OK;
}
int port_m107_float_extreme(enum port_m107_fmt f,const uint16_t * const *v,size_t count,uint16_t *out,int max){
 if(!v||!out||!count||count>255u)return PORT_M107_BAD_ARG;
 const uint16_t *selected=v[0];
 int st=port_m107_float_valid(f,selected);if(st!=PORT_M107_OK)return st;
 for(size_t i=1;i<count;i++){
  int rel=0;st=port_m107_float_cmp(f,v[i],selected,&rel);
  if(st!=PORT_M107_OK)return st;
  if(max?(rel>0):(rel<0))selected=v[i];
 }
 for(unsigned i=0;i<word_count(f);i++)out[i]=selected[i];
 return PORT_M107_OK;
}
int port_m107_float_sign(enum port_m107_fmt f,const uint16_t *arg,const uint16_t *sign,uint16_t *out){
 if(!out)return PORT_M107_BAD_ARG;
 int st=port_m107_float_valid(f,arg),sign_st=port_m107_float_valid(f,sign);
 if(st!=PORT_M107_OK)return st;
 if(sign_st!=PORT_M107_OK)return sign_st;
 for(unsigned i=0;i<word_count(f);i++)out[i]=arg[i];
 if(is_zero(f,arg)){for(unsigned i=0;i<word_count(f);i++)out[i]=0;return PORT_M107_OK;}
 out[0]=(uint16_t)((arg[0]&UINT16_C(0x7fff))|(sign[0]&UINT16_C(0x8000)));
 return PORT_M107_OK;
}
int port_m107_float_conj(enum port_m107_fmt f,const uint16_t *pair,uint16_t *out){
 if(!pair || !out || f==PORT_M107_H)return PORT_M107_BAD_ARG;
 unsigned n=word_count(f);
 int st=port_m107_float_valid(f,pair);
 if(st!=PORT_M107_OK)return st;
 st=port_m107_float_valid(f,pair+n);
 if(st!=PORT_M107_OK)return st;
 for(unsigned i=0;i<2*n;i++)out[i]=pair[i];
 if(!is_zero(f,pair+n))out[n]^=UINT16_C(0x8000);
 else for(unsigned i=0;i<n;i++)out[n+i]=0;
 return PORT_M107_OK;
}
/* Bit 0 is the implied, normalized leading fraction bit (value one).
 * Next bits follow the VAX 16-bit word significance order. */
static unsigned sigbit(enum port_m107_fmt f,const uint16_t *v,unsigned i){
 if(i==0)return 1;
 unsigned p=shift(f); /* bits below exponent and sign in first word */
 if(i<=p)return (v[0]>>(p-i))&1u;
 i-=p+1;
 unsigned wi=1u+i/16u;
 return wi<word_count(f)?(v[wi]>>(15u-(i%16u)))&1u:0u;
}
int port_m107_float_rounded(enum port_m107_fmt f,const uint16_t *v,unsigned outbits,int32_t *out){
 if(!out || (outbits!=16u&&outbits!=32u))return PORT_M107_BAD_ARG;
 int st=port_m107_float_valid(f,v);
 if(st!=PORT_M107_OK)return st;
 if(is_zero(f,v)){*out=0;return PORT_M107_OK;}
 int e=(int)exponent(f,v)-(int)bias(f);
 uint32_t magnitude=0;
 unsigned sign=(v[0]>>15)&1u;
 if(e>=0){
  /* For exponents much larger than the whole fraction, low 32 bits are 0.
   * The condition flags still report overflow, as VAX CVTR does. */
  unsigned precision=word_count(f)*16u-16u + shift(f)+1u;
  if(e<=(int)(precision+32u)){
   for(int i=0;i<e;i++)magnitude=(magnitude<<1) |sigbit(f,v,(unsigned)i);
  }
  if((unsigned)e<precision && sigbit(f,v,(unsigned)e))magnitude++;
 }
 uint32_t converted=sign?0u-magnitude:magnitude;
 if(outbits==16u){*out=(int16_t)(uint16_t)converted;
  if(e>16 || (sign ? (magnitude>32768u) : (magnitude>32767u)))return PORT_M107_OVERFLOW;
 }else{
  *out=(int32_t)converted;
  if(e>=33 || (sign?(magnitude>UINT32_C(2147483648)):(magnitude>INT32_MAX)))return PORT_M107_OVERFLOW;
 }
 return PORT_M107_OK;
}
int port_m107_int16_extreme(const int16_t * const *v,size_t n,int max,int16_t *out){
 if(!v||!out||!n||n>255u||!v[0])return PORT_M107_BAD_ARG;
 int16_t x=*v[0];for(size_t i=1;i<n;i++){if(!v[i])return PORT_M107_BAD_ARG;
  if(max?(*v[i]>x):(*v[i]<x))x=*v[i];}
 *out=x;return PORT_M107_OK;
}
int port_m107_int32_extreme(const int32_t * const *v,size_t n,int max,int32_t *out){
 if(!v||!out||!n||n>255u||!v[0])return PORT_M107_BAD_ARG;
 int32_t x=*v[0];for(size_t i=1;i<n;i++){if(!v[i])return PORT_M107_BAD_ARG;
  if(max?(*v[i]>x):(*v[i]<x))x=*v[i];}
 *out=x;return PORT_M107_OK;
}
int port_m107_int16_sign(int16_t mag,int16_t sign,int16_t *out){
 if(!out)return PORT_M107_BAD_ARG;
 uint16_t v=(uint16_t)mag;
 if((mag<0)!=(sign<0))v=(uint16_t)(0u-v);
 *out=(int16_t)v;return PORT_M107_OK;
}
int port_m107_int32_sign(int32_t mag,int32_t sign,int32_t *out){
 if(!out)return PORT_M107_BAD_ARG;
 uint32_t v=(uint32_t)mag;
 if((mag<0)!=(sign<0))v=0u-v;
 *out=(int32_t)v;return PORT_M107_OK;
}
static int radix50_code(unsigned char c){
 if(c==' ')return 0;
 if(c>='A'&&c<='Z')return c-'A'+1;
 if(c=='$')return 27;
 if(c=='.')return 28;
 if(c>='0'&&c<='9')return c-'0'+30;
 return -1;
}
int port_m107_rad50_word(const char *s,size_t len,uint16_t *out,size_t *used){
 if(!s||!out)return PORT_M107_BAD_ARG;
 uint32_t word=0;size_t i=0;
 for(;i<3 && i<len;i++){
  int digit=radix50_code((unsigned char)s[i]);
  if(digit<0)break;
  word=word*40u+(uint32_t)digit;
 }
 if(used)*used=i;
 for(;i<3;i++)word*=40u;
 *out=(uint16_t)word;
 return PORT_M107_OK;
}
int port_m107_rad50_six(const char *s,uint32_t *out){
 if(!s||!out)return PORT_M107_BAD_ARG;
 uint16_t left,right;size_t used;
 int st=port_m107_rad50_word(s,3,&left,&used);if(st!=PORT_M107_OK)return st;
 st=port_m107_rad50_word(s+3,3,&right,&used);if(st!=PORT_M107_OK)return st;
 *out=((uint32_t)left<<16)|right;return PORT_M107_OK;
}
int port_m107_irad50(const char *s,size_t len,uint16_t *out,size_t cap,size_t *consumed){
 if(!s||!out||!consumed||len>SIZE_MAX-2u||cap<(len+2u)/3u)return PORT_M107_BAD_ARG;
 size_t n=0,offset=0;
 while(offset<len){size_t used=0;int st=port_m107_rad50_word(s+offset,len-offset,out+n,&used);
  if(st!=PORT_M107_OK)return st;
  offset+=used;if(used!=3 && offset<len)break;n++;
 }
 *consumed=offset;return PORT_M107_OK;
}
int port_m107_r50asc(const uint16_t *v,size_t n,char *dest,size_t cap){
 if(!v||!dest||n>SIZE_MAX/3u||cap<n*3u)return PORT_M107_BAD_ARG;
 for(size_t i=0;i<n;i++){
  unsigned value=v[i];if(value>=64000u)return PORT_M107_BAD_ARG;
  unsigned a=value/1600u,b=(value/40u)%40u,c=value%40u;
  unsigned digits[3]={a,b,c};
  for(unsigned j=0;j<3;j++){
   unsigned k=digits[j];
   dest[i*3u+j]=(char)(k==0?' ':k<=26?('A'+k-1):k==27?'$':k==28?'.':k==29?'?':('0'+k-30));
  }
 }
 return PORT_M107_OK;
}
