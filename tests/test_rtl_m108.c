/* All 38 original-specific modules have separately callable host tests.
 * This never claims their original VAX calling frames or guest execution. */
#include "vms_rtl_m108_modules.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
static void make(enum port_m107_fmt f,int32_t x,uint16_t *o){assert(port108_float_from_integer(f,x,o)==PORT108_OK);}
static void expect_i(enum port_m107_fmt f,const uint16_t *a,int32_t wanted){int32_t n=0;assert(port108_float_integer(f,a,&n)==PORT108_OK&&n==wanted);}
static void numerical(void){
 uint16_t a[8],b[8],out[8],f[2],uw=0;uint32_t ul=0;int16_t iw;int32_t il;
 assert(port108_mthabs_i2(-123,&iw)==PORT108_OK&&iw==123);
 assert(port108_mthabs_i4(INT32_MIN,&il)==PORT108_OK&&il==INT32_MIN);
 make(PORT_M107_F,-5,a);assert(port108_mthabs_f(PORT_M107_F,a,out)==PORT108_OK);expect_i(PORT_M107_F,out,5);
 const uint16_t half[2]={0x4040u,0}; /* 0.75 */
 const uint16_t onehalf[2]={0x40c0u,0};
 const uint16_t negative[2]={0xc0c0u,0};
 assert(port108_mthaint(onehalf,out)==PORT108_OK);expect_i(PORT_M107_F,out,1);
 assert(port108_mthanint(onehalf,out)==PORT108_OK);expect_i(PORT_M107_F,out,2);
 assert(port108_mthanint(negative,out)==PORT108_OK);expect_i(PORT_M107_F,out,-2);
 assert(port108_mthfloor(negative,out)==PORT108_OK);expect_i(PORT_M107_F,out,-2);
 assert(port108_mthfloor(half,out)==PORT108_OK);expect_i(PORT_M107_F,out,0);
 assert(port108_mthaint(negative,out)==PORT108_OK);expect_i(PORT_M107_F,out,-1);
 const enum port_m107_fmt modes[3]={PORT_M107_D,PORT_M107_G,PORT_M107_H};
 for(unsigned i=0;i<3;i++){
  enum port_m107_fmt k=modes[i];
  make(k,-7,a);
  int st=0;
  if(k==PORT_M107_D){st=port108_mthdint(a,out);assert(st==PORT108_OK);expect_i(k,out,-7);st=port108_mthdfloor(a,out);assert(st==PORT108_OK);st=port108_mthdnint(a,out);}
  if(k==PORT_M107_G){st=port108_mthgint(a,out);assert(st==PORT108_OK);expect_i(k,out,-7);st=port108_mthgfloor(a,out);assert(st==PORT108_OK);st=port108_mthgnint(a,out);}
  if(k==PORT_M107_H){st=port108_mthhint(a,out);assert(st==PORT108_OK);expect_i(k,out,-7);st=port108_mthhfloor(a,out);assert(st==PORT108_OK);st=port108_mthhnint(a,out);}
  assert(st==PORT108_OK);expect_i(k,out,-7);
 }
 make(PORT_M107_F,17,a);make(PORT_M107_F,5,b);
 assert(port108_mthamod(a,b,out)==PORT108_OK);expect_i(PORT_M107_F,out,2);
 assert(port108_mthamod(onehalf,b,out)==PORT108_PARTIAL);
 make(PORT_M107_D,17,a);make(PORT_M107_D,5,b);
 assert(port108_mthdmod(a,b,out)==PORT108_OK);expect_i(PORT_M107_D,out,2);
 make(PORT_M107_G,17,a);make(PORT_M107_G,5,b);
 assert(port108_mthgmod(a,b,out)==PORT108_OK);expect_i(PORT_M107_G,out,2);
 make(PORT_M107_H,17,a);make(PORT_M107_H,5,b);
 assert(port108_mthhmod(a,b,out)==PORT108_OK);expect_i(PORT_M107_H,out,2);
 assert(port108_mthmod_i2(-17,5,&iw)==PORT108_OK&&iw==-2);
 assert(port108_mthmod_i4(17,-5,&il)==PORT108_OK&&il==2);
 assert(port108_mthmod_i4(INT32_MIN,-1,&il)==PORT108_OVERFLOW);
 assert(port108_mthmod_i2(1,0,&iw)==PORT108_DIVZERO);
 assert(port108_mthdim_i2(-2,3,&iw)==PORT108_OK&&iw==0);
 assert(port108_mthdim_i2(8,3,&iw)==PORT108_OK&&iw==5);
 assert(port108_mthdim_i4(8,3,&il)==PORT108_OK&&il==5);
 make(PORT_M107_F,8,a);make(PORT_M107_F,3,b);
 assert(port108_mthdim_float(PORT_M107_F,a,b,out)==PORT108_OK);expect_i(PORT_M107_F,out,5);
 make(PORT_M107_F,3,a);make(PORT_M107_F,4,b);
 assert(port108_mthdprod(a,b,out)==PORT108_OK);expect_i(PORT_M107_D,out,12);
 assert(port108_mthgprod(a,b,out)==PORT108_OK);expect_i(PORT_M107_G,out,12);
 /* VAX F is an explicit, word-ordered format; no IEEE float casts. */
 assert(port108_mthbitops_i2(0,0xaaaa,0x0f0f,&uw)==PORT108_OK&&uw==0x0a0a);
 assert(port108_mthbitops_i2(1,0xaaaa,0x0f0f,&uw)==PORT108_OK&&uw==0xafaf);
 assert(port108_mthbitops_i4(2,0xff00,0x00ff,&ul)==PORT108_OK&&ul==0xffff);
 assert(port108_mthbitops_i2(3,0xaaaa,0,&uw)==PORT108_OK&&uw==0x5555);
 assert(port108_mthbitops_i4(4,0x80000000u,(uint32_t)-1,&ul)==PORT108_OK&&ul==0x40000000u);
 uint16_t pair[8];make(PORT_M107_F,3,pair);make(PORT_M107_F,4,pair+2);
 assert(port108_mthcabs(pair,out)==PORT108_OK);expect_i(PORT_M107_F,out,5);
 make(PORT_M107_D,3,pair);make(PORT_M107_D,4,pair+4);
 assert(port108_mthcdabs(pair,out)==PORT108_OK);expect_i(PORT_M107_D,out,5);
 make(PORT_M107_G,3,pair);make(PORT_M107_G,4,pair+4);
 assert(port108_mthcgabs(pair,out)==PORT108_OK);expect_i(PORT_M107_G,out,5);
 make(PORT_M107_F,2,pair);make(PORT_M107_F,2,pair+2);
 assert(port108_mthcabs(pair,out)==PORT108_PARTIAL);
 uint32_t seed=1;assert(port108_mthrandom(&seed,f)==PORT108_OK&&seed==69069u);
 uint16_t high=0,low=0;assert(port108_forrandom(&high,&low,f)==PORT108_OK&&low==3);
 assert(f[0]!=0); /* 3/2**31 must not be rounded to zero */
}
static void queue_test(void){
 uint8_t memory[96]={0};struct port108_queue q={memory,sizeof memory,0x1000u,0x1000u};uint32_t removed=0;
 assert(port108_queue_init(&q)==PORT108_OK);
 assert(port108_libinsqhi(&q,0x1008u)==PORT108_ONE);
 assert(port108_libinsqti(&q,0x1010u)==PORT108_OK);
 assert(port108_libinsqhi(&q,0x1018u)==PORT108_OK);
 assert(port108_libremqhi(&q,&removed)==PORT108_OK&&removed==0x1018u);
 assert(port108_libremqti(&q,&removed)==PORT108_OK&&removed==0x1010u);
 assert(port108_libremqhi(&q,&removed)==PORT108_ONE&&removed==0x1008u);
 assert(port108_libremqti(&q,&removed)==PORT108_EMPTY&&removed==0);
 assert(port108_libinsqhi(&q,0x1008u)==PORT108_ONE);
 assert(port108_libinsqhi(&q,0x1008u)==PORT108_BUSY);
 assert(port108_libinsqhi(&q,0x1003u)==PORT108_BAD_ARG);
 assert(port108_libinsqhi(&q,0x1004u)==PORT108_BAD_ARG);
 assert(port108_libinsqhi(NULL,0x1008u)==PORT108_BAD_ARG);
}
static void librtl(void){
 uint8_t data[16]={1,2,3,4,5,6,7,8,9,0,0,0,0,0,0,0};
 assert(port108_strmovq(data,data+2)==PORT108_OK);
 assert(data[2]==1&&data[9]==8);
 uint8_t tab[256];for(unsigned i=0;i<256;i++)tab[i]=(uint8_t)(i^0x20u);
 const uint8_t src[]={0x41,0x42,0x43};uint8_t dst[8]={0};
 assert(port108_libmovtc(src,3,tab,'.',dst,5)==PORT108_OK);
 assert(dst[0]==0x61&&dst[1]==0x62&&dst[2]==0x63&&dst[3]=='.'&&dst[4]=='.');
 size_t index=999;
 assert(port108_libmovtuc(src,3,tab,0x62,'.',dst,5,&index)==PORT108_OK);
 assert(index==2&&dst[0]==0x61&&dst[1]=='.');
 uint32_t crc=port108_libcrc(0xEDB88320u,0xffffffffu,(const uint8_t*)"123456789",9);
 assert(crc==0x340bc6d9u); /* raw reflected state, without final XOR */
 int64_t product=0;
 assert(port108_libemul(-4,8,5,&product)==PORT108_OK&&product==-27);
 int32_t q=0,r=0;
 assert(port108_libediv(7,100,&q,&r)==PORT108_OK&&q==14&&r==2);
 assert(port108_libediv(0,100,&q,&r)==PORT108_DIVZERO);
 assert(port108_libediv(1,(int64_t)INT32_MAX+1,&q,&r)==PORT108_OVERFLOW);
 assert(port108_libediv(-1,INT64_MIN,&q,&r)==PORT108_OVERFLOW);
 uint16_t opcode=0;const uint8_t code[2]={0xfd,0xaa};
 assert(port108_libgetopc(code,2,&opcode)==PORT108_OK&&opcode==0xaafdu);
 assert(port108_libgetopc(code,1,&opcode)==PORT108_BAD_ARG);
}
int main(void){numerical();queue_test();librtl();puts("M108: 38 native original-specific RTL modules, bounded host tests OK");return 0;}
