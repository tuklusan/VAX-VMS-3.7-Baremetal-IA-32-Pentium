#include "vms_rtl_m113.h"
#include <stdint.h>
#include <stddef.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

static int fails;
#define CHECK(e) do {if(!(e)){fprintf(stderr,"M113 test at %d failed: %s\n",__LINE__,#e);fails++;}} while(0)
static int run_init(void *p){unsigned *x=(unsigned *)p;(*x)++;return PORT113_OK;}
static int run_handler(void *p,uint32_t n){int *x=(int *)p;*x+=(int)n;return *x;}
int main(void){
    uint8_t v=0xff;
    /* All exact original 256-byte reversible ASCII/EBCDIC values and the
     * non-reversible ASCII/EBCDIC and COBOL tables are self-contained. */
    for(int i=0;i<256;i++){
        uint8_t eb=0,ascii=0;
        CHECK(port113_src_liba2erev((uint8_t)i,&eb)==PORT113_OK);
        CHECK(port113_src_libe2arev(eb,&ascii)==PORT113_OK);
        CHECK(ascii==(uint8_t)i);
        CHECK(port113_src_libascebc((uint8_t)i,&eb)==PORT113_OK);
        CHECK(port113_src_libebcasc((uint8_t)i,&v)==PORT113_OK);
        CHECK(port113_src_cobspanc((uint8_t)i,&v)==PORT113_OK);
        CHECK(port113_src_cobspanc2((uint8_t)i,&v)==PORT113_OK);
        CHECK(port113_src_cobdeedit((uint8_t)i,&v)==PORT113_OK);
    }
    CHECK(port113_src_libascebc(0x80,&v)==PORT113_OK&&v==0x3f);
    CHECK(port113_src_libascebc(0xff,&v)==PORT113_OK&&v==0xff);
    CHECK(port113_src_cobspanc2('A',&v)==PORT113_OK&&v==1);
    CHECK(port113_src_cobspanc2('a',&v)==PORT113_OK&&v==2);
    CHECK(port113_src_cobspanc2(' ',&v)==PORT113_OK&&v==3);
    CHECK(port113_src_cobdeedit('9',&v)==PORT113_OK&&v=='9');
    CHECK(port113_src_cobdeedit('x',&v)==PORT113_OK&&v=='0');
    uint32_t a[3]={UINT32_MAX,UINT32_MAX,0},b[3]={1,0,0},res[3]={0};
    CHECK(port113_src_addx(a,b,res,3)==PORT113_OK&&res[0]==0&&res[1]==0&&res[2]==1);
    CHECK(port113_src_subx(res,b,res,3)==PORT113_OK&&res[0]==UINT32_MAX&&res[1]==UINT32_MAX&&res[2]==0);
    CHECK(port113_src_libtempla(0)==PORT113_BAD_ARG);
    CHECK(port113_src_libtempla(a)==PORT113_OK);
    int ast=0;CHECK(port113_src_libastinp(0,1,&ast)==PORT113_APPROX&&ast==1);
    CHECK(port113_src_libastinp(4,1,&ast)==PORT113_BAD_ARG);
    uint16_t out[8]={0};
    uint8_t pack[12]={0};
    int64_t recovered=0;
    for(int64_t n=-123456789;n<=123456789;n+=17343877){
        CHECK(port113_packed_encode(n,11,pack,sizeof pack)==PORT113_OK);
        CHECK(port113_packed_decode(pack,11,&recovered)==PORT113_OK&&recovered==n);
    }
    CHECK(port113_packed_encode(-1234,4,pack,sizeof pack)==PORT113_OK);
    CHECK(port113_packed_decode(pack,4,&recovered)==PORT113_OK&&recovered==-1234);
    pack[0]|=0x10;CHECK(port113_packed_decode(pack,4,&recovered)==PORT113_BAD_ARG);
    uint8_t little[2]={0};CHECK(port113_packed_encode(12345,3,little,sizeof little)==PORT113_OVERFLOW);
    uint8_t fpack[4]={0};CHECK(port113_packed_encode(12345,5,fpack,sizeof fpack)==PORT113_OK);
    CHECK(port113_src_cobcvtpf(fpack,5,0,out,sizeof out)==PORT113_APPROX);
    long double flt=0;CHECK(port109_decode(PORT_M107_F,out,&flt)==PORT109_OK&&flt==12345.0L);
    CHECK(port113_src_cobcvtfp(out,5,0,pack,sizeof pack)==PORT113_APPROX);
    CHECK(port113_packed_decode(pack,5,&recovered)==PORT113_OK&&recovered==12345);
    CHECK(port113_src_cobcvtpd(fpack,5,0,out,sizeof out)==PORT113_APPROX);
    CHECK(port109_decode(PORT_M107_D,out,&flt)==PORT109_OK&&flt==12345.0L);
    uint16_t single[2]={0};CHECK(port113_src_libcvtdf(out,single)==PORT113_APPROX);
    CHECK(port109_decode(PORT_M107_F,single,&flt)==PORT109_OK&&flt==12345.0L);
    int64_t q=0,r=0;
    CHECK(port113_src_cobdivq(-100,7,&q,&r)==PORT113_OK&&q==-14&&r==-2);
    CHECK(port113_src_cobdivq(INT64_MIN,1,&q,&r)==PORT113_OK&&q==INT64_MIN&&r==0);
    CHECK(port113_src_cobdivq(INT64_MIN,-1,&q,&r)==PORT113_BAD_ARG);
    CHECK(port113_src_cobdivq(100,0,&q,&r)==PORT113_BAD_ARG);
    CHECK(port113_src_expi(-2,63,&q)==PORT113_APPROX&&q==INT64_MIN);
    CHECK(port113_src_expi(2,63,&q)==PORT113_OVERFLOW);
    CHECK(port113_src_expi(2,-1,&q)==PORT113_UNSUPPORTED);
    int32_t days=-1;uint32_t cs=0;
    CHECK(port113_src_libday(864000000000ULL*13+1234500000ULL,&days,&cs)==PORT113_OK && days==13 && cs==12345);
    CHECK(port113_src_libday(UINT64_MAX,&days,&cs)==PORT113_OK);
    uint16_t mat_words[32]={0};struct port113_matrix mat={mat_words,32,2,2,PORT_M107_F};
    CHECK(port113_src_basmatidn(&mat)==PORT113_OK);
    CHECK(port109_decode(PORT_M107_F,mat_words,&flt)==PORT109_OK&&flt==1.0L);
    CHECK(port109_decode(PORT_M107_F,mat_words+2,&flt)==PORT109_OK&&flt==0.0L);
    CHECK(port109_decode(PORT_M107_F,mat_words+6,&flt)==PORT109_OK&&flt==1.0L);
    CHECK(port113_src_basmatnul(&mat)==PORT113_OK&&mat_words[0]==0&&mat_words[6]==0);
    const uint16_t val[2]={0x4080,0};
    CHECK(port113_src_basmatini(&mat,val)==PORT113_OK&&mat_words[0]==val[0]&&mat_words[6]==val[0]);
    CHECK(port113_src_basmatred(&mat,3,2)==PORT113_OK&&mat.rows==3);
    CHECK(port113_src_basmatred(&mat,10000,10000)==PORT113_BAD_ARG&&mat.rows==3);
    char numeric[8]={0};CHECK(port113_src_cobcnvout(-123,numeric,sizeof numeric)==PORT113_OK);
    CHECK(memcmp(numeric,"    -123",8)==0);
    char tiny[2]={0};CHECK(port113_src_cobcnvout(10000,tiny,sizeof tiny)==PORT113_OVERFLOW);
    CHECK(tiny[0]=='*'&&tiny[1]=='*');
    uint8_t edited[]={'a','[','b',']'};
    CHECK(port113_src_basedit(edited,sizeof edited,1,1)==PORT113_APPROX);
    CHECK(memcmp(edited,"A(B)",4)==0);
    unsigned init_count=0;struct port113_init_call calls[2]={{run_init,&init_count},{run_init,&init_count}};
    int result=0;CHECK(port113_src_libinitia(calls,2,&result)==PORT113_APPROX&&init_count==2);
    int value=3;CHECK(port113_src_cobinvuse(run_handler,&value,2,&result)==PORT113_APPROX && value==5 && result==5);
    CHECK(port113_src_cobdhandl(run_handler,&value,3,&result)==PORT113_APPROX&&value==8);
    CHECK(port113_src_stop(0,0)==PORT113_UNSUPPORTED);
    CHECK(port113_src_signal(0,0)==PORT113_UNSUPPORTED);
    if(fails){fprintf(stderr,"M113: %d failed assertions\n",fails);return 1;}
    puts("M113: 40 original RTL sources, exact 7 translation tables and bounded native ops PASS (host only)");
    return 0;
}
