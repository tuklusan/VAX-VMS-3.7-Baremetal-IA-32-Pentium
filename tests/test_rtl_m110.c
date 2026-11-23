#include "../src/vms_rtl_m110.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static const char *stage="unknown";
static const enum port_m107_fmt fmt[]={PORT_M107_F,PORT_M107_D,PORT_M107_G,PORT_M107_H};
static int ok(int result){return result==PORT109_APPROX||result==PORT109_OK;}
static long double absld(long double x){return x<0.0L?-x:x;}
static void check(enum port_m107_fmt f,const uint16_t *a,long double v){
    long double x=0.0L;
    assert(port109_decode(f,a,&x)==PORT109_OK);
    if(!(absld(x-v)<0.000006L*(1.0L+absld(v)))){fprintf(stderr,"Mismatch %s fmt=%d actual=%Lg expected=%Lg\n",stage,f,x,v);assert(0);}
}
static unsigned words(enum port_m107_fmt f){return f==PORT_M107_F?2u:f==PORT_M107_H?8u:4u;}
static void from(enum port_m107_fmt f,long double x,uint16_t *out){assert(port109_encode(f,x,out)==PORT109_OK);}

static void complex_tests(void){
    for(unsigned k=0;k<3;k++){
        enum port_m107_fmt f=fmt[k];const unsigned n=words(f);
        uint16_t x[16]={0},out[16]={0},pristine[16];
        int(*exp)(const uint16_t*,uint16_t*)=(k==0?port110_mthcexp:k==1?port110_mthcdexp:port110_mthcgexp);
        int(*log)(const uint16_t*,uint16_t*)=(k==0?port110_mthclog:k==1?port110_mthcdlog:port110_mthcglog);
        int(*sqrtf)(const uint16_t*,uint16_t*)=(k==0?port110_mthcsqrt:k==1?port110_mthcdsqrt:port110_mthcgsqrt);
        int(*sin)(const uint16_t*,uint16_t*)=(k==0?port110_mthcsinco_sin:k==1?port110_mthcdsinc_sin:port110_mthcgsinc_sin);
        int(*cos)(const uint16_t*,uint16_t*)=(k==0?port110_mthcsinco_cos:k==1?port110_mthcdsinc_cos:port110_mthcgsinc_cos);
        stage="complex exp";assert(ok(exp(x,out)));check(f,out,1.0L);check(f,out+n,0.0L);
        from(f,1.0L,x);stage="complex log";assert(ok(log(x,out)));check(f,out,0.0L);check(f,out+n,0.0L);
        memset(x,0,sizeof x);memset(out,0x5a,sizeof out);memcpy(pristine,out,sizeof out);
        assert(log(x,out)==PORT109_DOMAIN && !memcmp(out,pristine,sizeof out));
        from(f,4.0L,x);stage="complex sqrt";assert(ok(sqrtf(x,out)));check(f,out,2.0L);check(f,out+n,0.0L);
        from(f,-1.0L,x);stage="complex sqrt";assert(ok(sqrtf(x,out)));check(f,out,0.0L);check(f,out+n,1.0L);
        memset(x,0,sizeof x);stage="complex sin";assert(ok(sin(x,out)));check(f,out,0.0L);check(f,out+n,0.0L);
        stage="complex cos";assert(ok(cos(x,out)));check(f,out,1.0L);check(f,out+n,0.0L);
        assert(sin(NULL,out)==PORT109_BAD_ARG);
        from(f,0.0L,x);from(f,1.0L,x+n);
        stage="complex sin";assert(ok(sin(x,out)));check(f,out,0.0L);check(f,out+n,1.17520119364380145L);
    }
}
static void scalar_tests(void){
    for(unsigned k=0;k<4;k++){
        enum port_m107_fmt f=fmt[k];unsigned n=words(f);
        uint16_t x[8]={0},s[8]={0},c[8]={0};
        int(*atanh)(const uint16_t*,uint16_t*)=(k==0?port110_mthatanh:k==1?port110_mthdatanh:k==2?port110_mthgatanh:port110_mthhatanh);
        int(*sincos)(const uint16_t*,uint16_t*,uint16_t*)=(k==0?port110_mthsincos:k==1?port110_mthdsinco:k==2?port110_mthgsinco:port110_mthhsinco);
        int(*sincosd)(const uint16_t*,uint16_t*,uint16_t*)=(k==0?port110_mthsincosd:k==1?port110_mthdsincod:k==2?port110_mthgsincod:port110_mthhsincod);
        int(*poly)(const uint16_t*,unsigned,const uint16_t*,uint16_t*)=(k==0?port110_libpolyf:k==1?port110_libpolyd:k==2?port110_libpolyg:port110_libpolyh);
        int(*emod)(const uint16_t*,uint16_t,const uint16_t*,int32_t*,uint16_t*)=(k==0?port110_libemodf:k==1?port110_libemodd:k==2?port110_libemodg:port110_libemodh);
        from(f,0.0L,x);assert(atanh(x,s)==PORT109_OK);check(f,s,0.0L);
        from(f,0.5L,x);assert(ok(atanh(x,s)));check(f,s,0.549306144334054845L);
        from(f,1.0L,x);memset(s,0x55,sizeof s);assert(atanh(x,s)==PORT109_DOMAIN);for(unsigned i=0;i<8;i++)assert(s[i]==0x5555);
        from(f,0.0L,x);stage="sincos";assert(ok(sincos(x,s,c)));check(f,s,0.0L);check(f,c,1.0L);
        from(f,90.0L,x);stage="sincosd";assert(ok(sincosd(x,s,c)));check(f,s,1.0L);check(f,c,0.0L);
        uint16_t coef[24]={0};
        from(f,3.0L,coef);from(f,4.0L,coef+n);from(f,5.0L,coef+2*n);
        from(f,2.0L,x);assert(ok(poly(x,2u,coef,s)));check(f,s,31.0L);
        assert(poly(x,33u,coef,s)==PORT109_BAD_ARG);
        int32_t ip=777;
        from(f,3.0L,x);from(f,2.0L,c);
        assert(emod(x,1u,c,&ip,s)==PORT109_PARTIAL&&ip==777);
        assert(ok(emod(x,0u,c,&ip,s))&&ip==6);check(f,s,0.0L);
    }
}
static void conversion_tests(void){
    uint16_t f[8]={0},d[8]={0},g[8]={0};
    int32_t i=0;
    assert(ok(port110_mthconver_dfloti(-23,d)));check(PORT_M107_D,d,-23.0L);
    assert(ok(port110_mthconver_dflotj(1234567,d)));check(PORT_M107_D,d,1234567.0L);
    assert(ok(port110_mthcvtdg_d_g(d,g)));check(PORT_M107_G,g,1234567.0L);
    assert(ok(port110_mthcvtdg_g_d(g,d)));check(PORT_M107_D,d,1234567.0L);
    from(PORT_M107_F,-23.0L,f);
    assert(ok(port110_mthconver_iint(f,&i))&&i==-23);
    assert(ok(port110_mthconver_jint(f,&i))&&i==-23);
    assert(ok(port110_mthconver_dble(f,d)));check(PORT_M107_D,d,-23.0L);
    assert(ok(port110_mthconver_sngl(d,f)));check(PORT_M107_F,f,-23.0L);
    from(PORT_M107_F,33000.0L,f);
    i=777;assert(port110_mthconver_iint(f,&i)==PORT109_RANGE&&i==777);
}
static void translation_tests(void){
    uint8_t text[]={"Az9 01"},out[16]={0},back[16]={0};
    assert(ok(port110_libtraa2e(text,6,out,sizeof out)));
    assert(out[0]==0xc1&&out[1]==0xa9&&out[2]==0xf9&&out[3]==0x40);
    assert(ok(port110_libtrae2a(out,6,back,sizeof back)));
    assert(!memcmp(text,back,6));
    text[2]='\n';memset(out,0x55,sizeof out);
    assert(port110_libtraa2e(text,6,out,sizeof out)==PORT109_PARTIAL);
    for(unsigned i=0;i<16;i++)assert(out[i]==0x55);
    assert(port110_libtraa2e(text,6,out,5)==PORT109_BAD_ARG);
}
static void pool_tests(void){
    struct port110_ef_pool ef;struct port110_lun_pool lun;
    port110_ef_init(&ef);port110_lun_init(&lun);
    unsigned f=0,n=0;
    assert(port110_libef_get(&ef,&f)==PORT109_OK&&f==63);
    assert(port110_libef_get(&ef,&f)==PORT109_OK&&f==62);
    assert(port110_libef_free(&ef,24)==PORT109_BAD_ARG);
    assert(port110_libef_free(&ef,63)==PORT109_OK);
    assert(port110_libef_reserve(&ef,63)==PORT109_OK);
    assert(port110_libef_reserve(&ef,63)==PORT109_PARTIAL);
    for(unsigned i=0;i<30;i++)assert(port110_libef_get(&ef,&f)==PORT109_OK);
    assert(port110_libef_get(&ef,&f)==PORT109_PARTIAL);
    assert(port110_liblun_get(&lun,&n)==PORT109_OK&&n==119);
    assert(port110_liblun_free(&lun,119)==PORT109_OK);
    assert(port110_liblun_get(&lun,&n)==PORT109_OK&&n==119);
    for(unsigned i=0;i<19;i++)assert(port110_liblun_get(&lun,&n)==PORT109_OK);
    assert(port110_liblun_get(&lun,&n)==PORT109_PARTIAL);
}
int main(void){complex_tests();scalar_tests();conversion_tests();translation_tests();pool_tests();
    puts("M110: 34 additional original RTL modules and 2 SYS line mappings — host checks PASS");
    return 0;
}
