/* Original source specific native IA-32 helper regression, not a guest boot. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "vms_rtl_m107.h"
static void round_tests(void){
 int32_t v=0;
 const uint16_t f_one[2]={0x4080,0};
 const uint16_t f_onehalf[2]={0x40c0,0};
 const uint16_t f_minus[2]={0xc0c0,0};
 const uint16_t d_half[4]={0x40c0,0,0,0};
 const uint16_t g_half[4]={0x4018,0,0,0};
 const uint16_t h_half[8]={0x4001,0x8000,0,0,0,0,0,0};
 assert(port_m107_mthinint(f_one,&v)==PORT_M107_OK&&v==1);
 assert(port_m107_mthjnint(f_onehalf,&v)==PORT_M107_OK&&v==2);
 assert(port_m107_mthinint(f_minus,&v)==PORT_M107_OK&&v==-2);
 assert(port_m107_mthiidnnt(d_half,&v)==PORT_M107_OK&&v==2);
 assert(port_m107_mthjidnnt(d_half,&v)==PORT_M107_OK&&v==2);
 assert(port_m107_mthiignnt(g_half,&v)==PORT_M107_OK&&v==2);
 assert(port_m107_mthjignnt(g_half,&v)==PORT_M107_OK&&v==2);
 assert(port_m107_mthiihnnt(h_half,&v)==PORT_M107_OK&&v==2);
 assert(port_m107_mthjihnnt(h_half,&v)==PORT_M107_OK&&v==2);
 const uint16_t f_hf[2]={0x4000,0};
 const uint16_t f_mhf[2]={0xc000,0};
 assert(port_m107_mthjnint(f_hf,&v)==PORT_M107_OK&&v==1);
 assert(port_m107_mthjnint(f_mhf,&v)==PORT_M107_OK&&v==-1);
 const uint16_t f_too_large[2]={0x5000,0};
 assert(port_m107_mthjnint(f_too_large,&v)==PORT_M107_OVERFLOW&&v==INT32_MIN);
 const uint16_t f_very_large[2]={0x5100,0};
 assert(port_m107_mthinint(f_very_large,&v)==PORT_M107_OVERFLOW);
 const uint16_t f_min[2]={0xd000,0};
 assert(port_m107_mthjnint(f_min,&v)==PORT_M107_OK&&v==INT32_MIN);
 const uint16_t reserved[2]={0x8000,0};
 assert(port_m107_mthjnint(reserved,&v)==PORT_M107_RESERVED_FLOAT);
}
static void extrema(void){
 int16_t a=-4,b=20,c=7,res16=0; const int16_t *w[3]={&a,&b,&c};
 assert(port_m107_mthimax0(w,3,&res16)==PORT_M107_OK&&res16==20);
 assert(port_m107_mthimin0(w,3,&res16)==PORT_M107_OK&&res16==-4);
 assert(port_m107_mthimax0(w,0,&res16)==PORT_M107_BAD_ARG);
 int32_t x=-100000,y=220000,z=99,res32=0; const int32_t *l[3]={&x,&y,&z};
 assert(port_m107_mthjmax0(l,3,&res32)==PORT_M107_OK&&res32==y);
 assert(port_m107_mthjmin0(l,3,&res32)==PORT_M107_OK&&res32==x);
 int16_t sr16;int32_t sr32;
 assert(port_m107_mthiisign(-19,2,&sr16)==PORT_M107_OK&&sr16==19);
 assert(port_m107_mthiisign(19,-2,&sr16)==PORT_M107_OK&&sr16==-19);
 assert(port_m107_mthiisign(INT16_MIN,2,&sr16)==PORT_M107_OK&&sr16==INT16_MIN);
 assert(port_m107_mthjisign(-190,2,&sr32)==PORT_M107_OK&&sr32==190);
 assert(port_m107_mthjisign(190,-2,&sr32)==PORT_M107_OK&&sr32==-190);
 assert(port_m107_mthjisign(INT32_MIN,2,&sr32)==PORT_M107_OK&&sr32==INT32_MIN);
}
static void float_extrema_sign(void){
 const uint16_t fa[2]={0x4080,0},fb[2]={0xc080,0},fc[2]={0x4100,0},fz[2]={0,0};
 const uint16_t *fv[4]={fa,fb,fc,fz}; uint16_t out[16]={0};
 assert(port_m107_mthmax1(fv,4,out)==PORT_M107_OK && out[0]==fc[0]);
 assert(port_m107_mthmin1(fv,4,out)==PORT_M107_OK && out[0]==fb[0]);
 assert(port_m107_mthsign(fc,fb,out)==PORT_M107_OK && out[0]==0xc100);
 assert(port_m107_mthsign(fz,fb,out)==PORT_M107_OK && out[0]==0);
 int sign=3;
 assert(port_m107_mthsgn(fb,&sign)==PORT_M107_OK && sign==-1);
 assert(port_m107_mthsgn(fa,&sign)==PORT_M107_OK && sign==1);
 assert(port_m107_mthsgn(fz,&sign)==PORT_M107_OK && sign==0);
 const uint16_t da[4]={0x4080,0,0,0},db[4]={0xc080,0,0,0},dc[4]={0x4100,0,0,0};
 const uint16_t *dv[3]={da,db,dc};
 assert(port_m107_mthdmax1(dv,3,out)==PORT_M107_OK && out[0]==dc[0]);
 assert(port_m107_mthdmin1(dv,3,out)==PORT_M107_OK && out[0]==db[0]);
 assert(port_m107_mthdsign(dc,db,out)==PORT_M107_OK && out[0]==0xc100);
 const uint16_t ga[4]={0x4010,0,0,0},gb[4]={0xc010,0,0,0},gc[4]={0x4020,0,0,0};
 const uint16_t *gv[3]={ga,gb,gc};
 assert(port_m107_mthgmax1(gv,3,out)==PORT_M107_OK && out[0]==gc[0]);
 assert(port_m107_mthgmin1(gv,3,out)==PORT_M107_OK && out[0]==gb[0]);
 assert(port_m107_mthgsign(gc,gb,out)==PORT_M107_OK && out[0]==0xc020);
 const uint16_t ha[8]={0x4001,0,0,0,0,0,0,0},hb[8]={0xc001,0,0,0,0,0,0,0},hc[8]={0x4002,0,0,0,0,0,0,0};
 const uint16_t *hv[3]={ha,hb,hc};
 assert(port_m107_mthhmax1(hv,3,out)==PORT_M107_OK && out[0]==hc[0]);
 assert(port_m107_mthhmin1(hv,3,out)==PORT_M107_OK && out[0]==hb[0]);
 assert(port_m107_mthhsign(hc,hb,out)==PORT_M107_OK && out[0]==0xc002);
 const uint16_t invalid[4]={0x8000,0,0,0};
 const uint16_t *bad[2]={da,invalid};
 assert(port_m107_mthdmax1(bad,2,out)==PORT_M107_RESERVED_FLOAT);
 /* word-1 fraction is significant after word-0, not little-endian uint64 order. */
 const uint16_t alt[4]={0x4080,0x0200,0,0};
 const uint16_t *altpair[2]={da,alt};
 assert(port_m107_mthdmax1(altpair,2,out)==PORT_M107_OK && out[1]==0x0200);
}
static void conjugation(void){
 uint16_t out[8]={0};
 const uint16_t fc[4]={0x4080,0,0xc100,0};
 assert(port_m107_mthconjg(fc,out)==PORT_M107_OK && out[0]==0x4080 && out[2]==0x4100);
 const uint16_t dc[8]={0x4080,0,0,0,0x4100,0,0,0};
 assert(port_m107_mthdconjg(dc,out)==PORT_M107_OK && out[4]==0xc100);
 const uint16_t gc[8]={0x4010,0,0,0,0xc010,0,0,0};
 assert(port_m107_mthgconjg(gc,out)==PORT_M107_OK && out[4]==0x4010);
}
static void radix50(void){
 uint16_t w=0,words[4]={0}; size_t used=0;
 assert(port_m107_comr50wd("ABC",3,&w,&used)==PORT_M107_OK && w==1683 && used==3);
 assert(port_m107_comr50wd("A",1,&w,&used)==PORT_M107_OK && w==1600 && used==1);
 uint32_t packed=0;
 assert(port_m107_comrad50("ABCDEF",&packed)==PORT_M107_OK && packed==((UINT32_C(1683)<<16)|6606u));
 char output[16]={0};
 assert(port_m107_comr50asc(&w,1,output,sizeof(output))==PORT_M107_OK);
 assert(memcmp(output,"A  ",3)==0);
 assert(port_m107_comirad50("ABCDEF",6,words,4,&used)==PORT_M107_OK && used==6);
 assert(port_m107_comirad50("ABCD",4,words,4,&used)==PORT_M107_OK && used==4);
 assert(words[0]==1683 && words[1]==6400);
 assert(port_m107_comirad50("ABCDEF",6,words,4,&used)==PORT_M107_OK && used==6);
 assert(words[0]==1683 && words[1]==6606);
 assert(port_m107_comr50asc(words,2,output,sizeof(output))==PORT_M107_OK);
 assert(memcmp(output,"ABCDEF",6)==0);
 assert(port_m107_comr50asc(words,2,output,5)==PORT_M107_BAD_ARG);
}
int main(void){round_tests();extrema();float_extrema_sign();conjugation();radix50();puts("M107 34 original RTL file native tests passed (host, not guest)");return 0;}
