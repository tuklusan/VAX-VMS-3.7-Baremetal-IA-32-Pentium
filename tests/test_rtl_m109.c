#include "../src/vms_rtl_m109.h"
#include "../src/vms_rtl_m108.h"
#include <assert.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
typedef int (*unary)(const uint16_t*,uint16_t*);
typedef int (*binary)(const uint16_t*,const uint16_t*,uint16_t*);
static const enum port_m107_fmt fmts[]={PORT_M107_F,PORT_M107_D,PORT_M107_G,PORT_M107_H};
static const int xval[]={0,1,1,2,1,2,1,1,1};
static const long double expectation[]={
 1.5707963267948966192313216916L,1.5707963267948966192313216916L,
 0.7853981633974483096156608458L,0.6931471805599453094172321215L,
 2.7182818284590452353602874713L,1.4142135623730950488016887242L,
 1.1752011936438014568823818506L,1.5430806348152437784779056208L,
 0.7615941559557648881194582826L};
static unary all[9][4] = {
 { port109_mthacos, port109_mthdacos, port109_mthgacos, port109_mthhacos },
 { port109_mthasin, port109_mthdasin, port109_mthgasin, port109_mthhasin },
 { port109_mthatan, port109_mthdatan, port109_mthgatan, port109_mthhatan },
 { port109_mthalog, port109_mthdlog, port109_mthglog, port109_mthhlog },
 { port109_mthexp, port109_mthdexp, port109_mthgexp, port109_mthhexp },
 { port109_mthsqrt, port109_mthdsqrt, port109_mthgsqrt, port109_mthhsqrt },
 { port109_mthsinh, port109_mthdsinh, port109_mthgsinh, port109_mthhsinh },
 { port109_mthcosh, port109_mthdcosh, port109_mthgcosh, port109_mthhcosh },
 { port109_mthtanh, port109_mthdtanh, port109_mthgtanh, port109_mthhtanh },
};
static long double abs_ld(long double x){return x<0?-x:x;}
static int acceptable(int st){return st==PORT109_OK||st==PORT109_APPROX;}
static void value(enum port_m107_fmt f,const uint16_t *v,long double expected){
 long double decoded=0.0L;
 assert(port109_decode(f,v,&decoded)==PORT109_OK);
 long double tol=f==PORT_M107_F?0.000008L:0.000000000002L;
 assert(abs_ld(decoded-expected)<=tol*(1.0L+abs_ld(expected)));
}
static void modes(void){
 for(unsigned k=0;k<4;k++){
  enum port_m107_fmt f=fmts[k];
  uint16_t in[8]={0},out[8]={0};
  for(unsigned op=0;op<9;op++){
   assert(port108_float_from_integer(f,xval[op],in)==PORT108_OK);
   int st=all[op][k](in,out);
   assert(st==PORT109_APPROX);
   value(f,out,expectation[op]);
  }
  /* Exact, instruction-independent algebraic identities. */
  for(unsigned op=0;op<9;op++){
   int x=0;if(op==PORT109_ACOS||op==PORT109_LOG||op==PORT109_SQRT)x=1;
   if(op==PORT109_LOG)x=1;
   assert(port108_float_from_integer(f,x,in)==PORT108_OK);
   int st=all[op][k](in,out);
   assert(acceptable(st));
  }
  /* Nontrivial domain errors do not modify the output buffer. */
  for(unsigned i=0;i<8;i++)out[i]=0x4554u;
  assert(port108_float_from_integer(f,2,in)==PORT108_OK);
  assert(all[PORT109_ASIN][k](in,out)==PORT109_DOMAIN);
  for(unsigned i=0;i<8;i++)assert(out[i]==0x4554u);
  assert(port108_float_from_integer(f,0,in)==PORT108_OK);
  assert(all[PORT109_LOG][k](in,out)==PORT109_DOMAIN);
  assert(port108_float_from_integer(f,-1,in)==PORT108_OK);
  assert(all[PORT109_SQRT][k](in,out)==PORT109_DOMAIN);
  assert(!memcmp(out,(uint16_t[8]){0x4554,0x4554,0x4554,0x4554,0x4554,0x4554,0x4554,0x4554},sizeof out));
  in[0]=0x8000u;
  assert(all[PORT109_EXP][k](in,out)==PORT109_RESERVED);
  if(f==PORT_M107_H){
   assert(port108_float_from_integer(f,1,in)==PORT108_OK);
   in[7]=1u;
   assert(all[PORT109_EXP][k](in,out)==PORT109_PARTIAL);
  }
 }
}
static void extras(void){
 uint16_t in[8]={0},b[8]={0},out[8]={0};
 for(unsigned k=0;k<4;k++){
  enum port_m107_fmt f=fmts[k];
  assert(port108_float_from_integer(f,1,in)==PORT108_OK);
  assert(port108_float_from_integer(f,1,b)==PORT108_OK);
  unary acos_degrees[4]={port109_mthacosd, port109_mthdacosd, port109_mthgacosd, port109_mthhacosd};
  assert(acceptable(acos_degrees[k](in,out)));value(f,out,0.0L);
  unary asin_degrees[4]={port109_mthasind, port109_mthdasind, port109_mthgasind, port109_mthhasind};
  assert(acceptable(asin_degrees[k](in,out)));value(f,out,90.0L);
  unary atan_degrees[4]={port109_mthatand, port109_mthdatand, port109_mthgatand, port109_mthhatand};
  assert(acceptable(atan_degrees[k](in,out)));value(f,out,45.0L);
  binary two[4]={port109_mthatan2, port109_mthdatan2, port109_mthgatan2, port109_mthhatan2};
  binary degtwo[4]={port109_mthatand2, port109_mthdatand2, port109_mthgatand2, port109_mthhatand2};
  assert(acceptable(two[k](in,b,out)));
  value(f,out,0.78539816339744830961566L);
  assert(acceptable(degtwo[k](in,b,out)));value(f,out,45.0L);
  assert(port108_float_from_integer(f,0,b)==PORT108_OK);
  assert(acceptable(two[k](in,b,out)));value(f,out,1.570796326794896619L);
  assert(port108_float_from_integer(f,0,in)==PORT108_OK);
  assert(two[k](in,b,out)==PORT109_DOMAIN);
  assert(port108_float_from_integer(f,100,in)==PORT108_OK);
  unary log10s[4]={port109_mthalog10, port109_mthdlog10, port109_mthglog10, port109_mthhlog10};
  unary log2s[4]={port109_mthalog2, port109_mthdlog2, port109_mthglog2, port109_mthhlog2};
  assert(acceptable(log10s[k](in,out)));value(f,out,2.0L);
  assert(port108_float_from_integer(f,8,in)==PORT108_OK);
  assert(acceptable(log2s[k](in,out)));value(f,out,3.0L);
 }
}
static void contracts(void){
 uint16_t a[8]={0},b[8]={0};long double v=0.0L;
 assert(port109_unary(PORT_M107_F,PORT109_EXP,NULL,b)==PORT109_BAD_ARG);
 assert(port109_unary(PORT_M107_F,PORT109_EXP,a,NULL)==PORT109_BAD_ARG);
 assert(port109_unary(0,PORT109_EXP,a,b)==PORT109_BAD_ARG);
 assert(port109_unary(PORT_M107_F,(enum port109_function)999,a,b)==PORT109_BAD_ARG);
 assert(port109_unary_degrees(PORT_M107_F,PORT109_COSH,a,b)==PORT109_BAD_ARG);
 assert(port109_atan_ratio(PORT_M107_F,a,a,2,b)==PORT109_BAD_ARG);
 assert(port109_decode(PORT_M107_F,NULL,&v)==PORT109_BAD_ARG);
 assert(port109_encode(PORT_M107_F,1.0L,NULL)==PORT109_BAD_ARG);
 assert(port108_float_from_integer(PORT_M107_F,1,a)==PORT108_OK);
 assert(port109_decode(PORT_M107_F,a,&v)==PORT109_OK && v==1.0L);
 assert(port109_encode(PORT_M107_F,1.0L,b)==PORT109_OK);
 assert(memcmp(a,b,4)==0);
 assert(port109_encode(PORT_M107_F,0.0L,b)==PORT109_OK && b[0]==0 && b[1]==0);
 assert(port109_encode(PORT_M107_F,1.0e100L,b)==PORT109_RANGE);
}
/* Differential raw-word roundtrip: 4,000 deterministic nonzero VAX
 * values per supported format. A silent word-layout error is a real bug. */
static uint32_t random32(uint32_t *seed){
 *seed=*seed*UINT32_C(1664525)+UINT32_C(1013904223);return *seed;
}
static void raw_roundtrip(void){
 uint32_t seed=UINT32_C(0x12345678);
 for(unsigned format=0;format<4;format++){
  enum port_m107_fmt f=fmts[format];
  const unsigned n=f==PORT_M107_F?2u:f==PORT_M107_H?8u:4u;
  const unsigned shift=f==PORT_M107_G?4u:f==PORT_M107_H?0u:7u;
  const unsigned biased=f==PORT_M107_G?1024u:f==PORT_M107_H?16384u:128u;
  for(unsigned iter=0;iter<4000u;iter++){
   uint16_t v[8]={0},w[8]={0};
   for(unsigned i=0;i<n;i++)v[i]=(uint16_t)(random32(&seed)>>16);
   unsigned expn=f==PORT_M107_H?biased+(random32(&seed)%513u)-256u:
          1u+(random32(&seed)%((f==PORT_M107_G)?2047u:255u));
   v[0]=(uint16_t)((v[0]&0x8000u)|((expn<<shift)&0x7fffu)|
                   (v[0]&((1u<<shift)-1u)));
   if(f==PORT_M107_H){v[4]&=0xfffeu;v[5]=v[6]=v[7]=0;}
   long double value=0.0L;
   assert(port109_decode(f,v,&value)==PORT109_OK);
   assert(port109_encode(f,value,w)==PORT109_OK);
   assert(memcmp(v,w,n*sizeof(uint16_t))==0);
  }
 }
}

int main(void){modes();extras();contracts();raw_roundtrip();
 puts("M109: 36 native original math modules, 64 entry wrappers, constrained host tests passed");return 0;}
