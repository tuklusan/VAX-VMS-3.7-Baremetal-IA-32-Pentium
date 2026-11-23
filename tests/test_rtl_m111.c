#include "../src/vms_rtl_m111.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
static const enum port_m107_fmt fmts[4]={PORT_M107_F,PORT_M107_D,PORT_M107_G,PORT_M107_H};
static unsigned nwords(enum port_m107_fmt fmt){return fmt==PORT_M107_F?2:fmt==PORT_M107_H?8:4;}
static int success(int s){return s==PORT109_OK||s==PORT109_APPROX;}
static long double dist(long double a,long double b){return a>b?a-b:b-a;}
static void init(enum port_m107_fmt f,long double v,uint16_t *p){
    assert(port109_encode(f,v,p)==PORT109_OK);
}
static void expect(enum port_m107_fmt f,const uint16_t *p,long double value){
    long double found=0;assert(port109_decode(f,p,&found)==PORT109_OK);
    if(!(dist(found,value)<0.000008L*(1.0L+(value<0?-value:value)))){
        fprintf(stderr,"Numeric F/D/G/H check failed: fmt=%u actual=%.12Lg expected=%.12Lg\n",(unsigned)f,found,value);
        assert(0);
    }
}
static void clock_tests(void){
    struct port111_clock t={2026,11,20,9,7,5,50};char data[24];
    assert(port111_clock_valid(&t));
    memset(data,'#',sizeof data);
    assert(port111_fortime(&t,data)==PORT109_OK&&!memcmp(data,"09:07:05",8)&&data[8]=='#');
    assert(port111_fordate(&t,data)==PORT109_OK&&!memcmp(data,"20-NOV-26",9));
    assert(port111_fortimeds(&t,data,12)==PORT109_OK&&!memcmp(data,"09:07:05    ",12));
    assert(port111_fordateds(&t,data,12)==PORT109_OK&&!memcmp(data,"20-NOV-26   ",12));
    assert(port111_fordateds(&t,data,4)==PORT109_OK&&!memcmp(data,"20-N",4));
    int16_t m=0,d=0,y=0;int32_t m4=0,d4=0,y4=0;
    assert(port111_foridate(&t,&m,&d,&y)==PORT109_OK&&m==11&&d==20&&y==26);
    assert(port111_forjdate(&t,&m4,&d4,&y4)==PORT109_OK&&m4==11&&d4==20&&y4==26);
    t.month=2;t.day=29;t.year=2024;assert(port111_clock_valid(&t));
    t.year=2025;assert(!port111_clock_valid(&t));
    char unchanged[12];memset(unchanged,'!',sizeof unchanged);
    assert(port111_fortimeds(&t,unchanged,12)==PORT109_BAD_ARG&&unchanged[0]=='!');
    t.year=2026;t.month=11;t.day=20;t.hour=0;t.minute=0;t.second=5;t.centisecond=0;
    uint16_t start[2]={0},out[2]={0};init(PORT_M107_F,86399.0L,start);
    assert(success(port111_forsecnds(&t,start,out)));expect(PORT_M107_F,out,6.0L);
    init(PORT_M107_F,1.0L,start);
    assert(success(port111_forsecnds(&t,start,out)));expect(PORT_M107_F,out,4.0L);
}
static void complex_tests(void){
    for(unsigned k=0;k<3;k++){
        const enum port_m107_fmt f=fmts[k];unsigned n=nwords(f);
        uint16_t a[16]={0},b[16]={0},o[16]={0};
        init(f,1,a);init(f,2,a+n);init(f,3,b);init(f,4,b+n);
        int(*mult)(const uint16_t*,const uint16_t*,uint16_t*)=k==1?port111_otsmulcd:k==2?port111_otsmulcg:NULL;
        int(*div)(const uint16_t*,const uint16_t*,uint16_t*)=k==0?port111_otsdivc:k==1?port111_otsdivcd:port111_otsdivcg;
        if(mult){assert(success(mult(a,b,o)));expect(f,o,-5);expect(f,o+n,10);}
        assert(success(div(a,b,o)));expect(f,o,11.0L/25.0L);expect(f,o+n,2.0L/25.0L);
        init(f,0,b);init(f,0,b+n);memset(o,0x5a,sizeof o);
        assert(div(a,b,o)==PORT109_DOMAIN);
        for(size_t i=0;i<sizeof o;i++)assert(((uint8_t*)o)[i]==0x5a);
        init(f,1,a);init(f,0,a+n);init(f,3,b);init(f,2,b+n);
        int(*cpow)(const uint16_t*,const uint16_t*,uint16_t*)=k==0?port111_otspowcc:k==1?port111_otspowcdc:port111_otspowcgc;
        assert(success(cpow(a,b,o)));expect(f,o,1);expect(f,o+n,0);
        init(f,0,a);memset(o,0x73,sizeof o);
        assert(cpow(a,b,o)==PORT109_DOMAIN); /* 0^(3+2i) non-real exponent */
        for(size_t i=0;i<sizeof o;i++)assert(((uint8_t*)o)[i]==0x73);
        init(f,2,a);init(f,1,a+n);
        int(*ipow)(const uint16_t*,int32_t,uint16_t*)=k==0?port111_otspowcj:k==1?port111_otspowcdj:port111_otspowcgj;
        assert(success(ipow(a,2,o)));expect(f,o,3);expect(f,o+n,4);
        assert(success(ipow(a,-1,o)));expect(f,o,0.4L);expect(f,o+n,-0.2L);
        memset(a,0,sizeof a);assert(ipow(a,-1,o)==PORT109_DOMAIN);
    }
    assert(port111_otsdivc(NULL,NULL,NULL)==PORT109_BAD_ARG);
}
static void realpow_tests(void){
    uint16_t a[8]={0},b[8]={0},o[8]={0};
    for(unsigned k=0;k<4;k++){
        const enum port_m107_fmt f=fmts[k];
        int(*powf)(const uint16_t*,const uint16_t*,uint16_t*)=k==0?port111_otspowrr:k==2?port111_otspowgg:k==3?port111_otspowhh:port111_otspowdd;
        init(f,2,a);init(f,3,b);
        assert(success(powf(a,b,o)));expect(f,o,8);
        init(f,-2,a);memset(o,0x11,sizeof o);
        assert(powf(a,b,o)==PORT109_DOMAIN);
        assert(o[0]==0x1111);
    }
    init(PORT_M107_D,2,a);init(PORT_M107_F,3,b);
    assert(success(port111_otspowdr(a,b,o)));expect(PORT_M107_D,o,8);
    init(PORT_M107_F,2,a);init(PORT_M107_D,3,b);
    assert(success(port111_otspowrd(a,b,o)));expect(PORT_M107_D,o,8);
    init(PORT_M107_F,-2,a);init(PORT_M107_F,3,b);
    assert(success(port111_baspowrr(a,b,o)));expect(PORT_M107_F,o,-8);
    assert(success(port111_baspowrj(a,3,o)));expect(PORT_M107_F,o,-8);
    assert(success(port111_baspowrj(a,-2,o)));expect(PORT_M107_F,o,0.25L);
    init(PORT_M107_F,0,a);
    assert(port111_baspowrj(a,-1,o)==PORT109_DOMAIN);
    init(PORT_M107_F,-2,a);init(PORT_M107_F,0.5L,b);
    assert(port111_baspowrr(a,b,o)==PORT109_DOMAIN);
    init(PORT_M107_D,-2,a);init(PORT_M107_D,3,b);
    assert(success(port111_baspowdd(a,b,o)));expect(PORT_M107_D,o,-8);
    assert(success(port111_baspowdj(a,3,o)));expect(PORT_M107_D,o,-8);
    assert(success(port111_baspowdj(a,-2,o)));expect(PORT_M107_D,o,0.25L);
    init(PORT_M107_D,-2,a);init(PORT_M107_D,0.5L,b);
    assert(port111_baspowdd(a,b,o)==PORT109_DOMAIN);
    assert(port111_baspowdj(NULL,3,o)==PORT109_BAD_ARG);
}
static void integer_tests(void){
    int16_t i16=777;int32_t i32=777;uint32_t u32=0;
    assert(port111_otspowii(3,4,&i16)==PORT109_OK&&i16==81);
    assert(port111_otspowjj(3,4,&i32)==PORT109_OK&&i32==81);
    assert(port111_baspowii(-2,3,&i16)==PORT109_OK&&i16==-8);
    assert(port111_baspowjj(-2,3,&i32)==PORT109_OK&&i32==-8);
    assert(port111_otspowii(-1,-3,&i16)==PORT109_OK&&i16==-1);
    assert(port111_otspowjj(-1,-4,&i32)==PORT109_OK&&i32==1);
    assert(port111_baspowjj(2,-3,&i32)==PORT109_OK&&i32==0);
    assert(port111_otspowii(0,0,&i16)==PORT109_DOMAIN&&i16==-1);
    assert(port111_otspowjj(0,-1,&i32)==PORT109_DOMAIN&&i32==0);
    assert(port111_baspowii(100,3,&i16)==PORT109_RANGE&&i16==-1);
    assert(port111_baspowjj(100000,3,&i32)==PORT109_RANGE&&i32==0);
    assert(port111_otspowlul(2,8,&u32)==PORT109_OK&&u32==256);
    assert(port111_otspowlul(0,0,&u32)==PORT109_DOMAIN&&u32==256);
    assert(port111_otspowlul(0xffffffffu,2,&u32)==PORT109_OK&&u32==1);
    assert(port111_otspowii(5,3,NULL)==PORT109_BAD_ARG);
}
static void lun_tests(void){
    struct port111_lun pool;port111_lun_init(&pool);
    unsigned num=101;
    for(unsigned i=0;i<100;i++)assert(port111_otslun_get(&pool,&num)==PORT109_OK&&num==i);
    assert(port111_otslun_get(&pool,&num)==PORT109_PARTIAL&&num==99);
    assert(port111_otslun_take(&pool,3)==PORT109_PARTIAL);
    assert(port111_otslun_free(&pool,3)==PORT109_OK);
    assert(port111_otslun_free(&pool,3)==PORT109_PARTIAL);
    assert(port111_otslun_take(&pool,3)==PORT109_OK);
    assert(port111_otslun_get(&pool,&num)==PORT109_PARTIAL);
    assert(port111_otslun_take(&pool,100)==PORT109_BAD_ARG);
}
static void parse_tests(void){
    int32_t x=123;int16_t shortv=123;int8_t bytev=12;
    assert(port111_otscvttll(" .TRUE.",7,32,&x)==PORT109_OK&&x==-1);
    assert(port111_otscvttll_compat(" .FALSE.",8,16,&shortv)==PORT109_OK&&shortv==0);
    assert(port111_otscvttll("   ",3,8,&bytev)==PORT109_OK&&bytev==0);
    assert(port111_otscvttll(".",1,32,&x)==PORT109_BAD_ARG&&x==-1);
    assert(port111_otscvttil("  -00012",8,32,0,&x)==PORT109_OK&&x==-12);
    assert(port111_otscvttil("   ",3,32,0,&x)==PORT109_OK&&x==0);
    assert(port111_otscvttil("-",1,32,0,&x)==PORT109_OK&&x==0);
    assert(port111_otscvttil_compat("\t+42",4,16,1,&shortv)==PORT109_OK&&shortv==42);
    assert(port111_otscvttil("\t+42",4,16,0,&shortv)==PORT109_BAD_ARG&&shortv==42);
    assert(port111_otscvttil("2147483648",10,32,0,&x)==PORT109_RANGE&&x==0);
    assert(port111_otscvttil("12",2,8,0,&bytev)==PORT109_OK&&bytev==12);
    assert(port111_otscvttil("300",3,8,0,&bytev)==PORT109_RANGE&&bytev==12);
    assert(port111_otscvttil("12A",3,32,0,&x)==PORT109_BAD_ARG&&x==0);
}
static void move_tests(void){
    char buf[32]="abcdefghij";
    assert(port111_otsmove(buf,buf+2,8)==PORT109_OK);
    assert(!memcmp(buf,"ababcdefgh",10));
    assert(port111_otsmove_r5(buf+2,buf,8)==PORT109_OK);
    assert(!memcmp(buf,"abcdefghgh",10));
    static uint8_t large[70040];
    for(size_t i=0;i<sizeof large;i++)large[i]=(uint8_t)(i%251u);
    assert(port111_otsmove(large,large+20,70000)==PORT109_OK);
    for(size_t i=0;i<70000;i++)assert(large[20+i]==(uint8_t)(i%251u));
    assert(port111_otsmove(NULL,NULL,1)==PORT109_BAD_ARG);
    assert(port111_otsmove(NULL,NULL,0)==PORT109_OK);
}
static void basic_tests(void){
    uint16_t a[8]={0},b[8]={0},out[8]={0};
    init(PORT_M107_D,2,a);init(PORT_M107_D,3,b);
    assert(success(port111_basmulddi_mul(a,b,out)));expect(PORT_M107_D,out,6);
    assert(success(port111_basmulddi_div(a,b,out)));expect(PORT_M107_D,out,2.0L/3.0L);
    init(PORT_M107_D,0,b);
    memset(out,0x48,sizeof out);
    assert(port111_basmulddi_div(a,b,out)==PORT109_DOMAIN&&out[0]==0x4848);
    init(PORT_M107_D,2,b);
    assert(port111_bascopyfd_d(b,out)==PORT109_OK&&!memcmp(b,out,8));
    init(PORT_M107_F,1.5L,b);
    assert(port111_bascopyfd_f(b,out)==PORT109_OK&&!memcmp(b,out,4));
    struct port111_det det={{0}};
    init(PORT_M107_D,5,b);
    assert(port111_basdet_store(&det,b)==PORT109_OK);
    assert(port111_basdet_f(&det,out)==PORT109_APPROX);expect(PORT_M107_F,out,5);
    assert(port111_basdet_d(&det,out)==PORT109_OK);expect(PORT_M107_D,out,5);
    assert(port111_basdet_store(NULL,b)==PORT109_BAD_ARG);
    uint8_t mapping[256]={0},txt[5]={'a','b','c','d','e'},dst[5]={0};size_t count=44;
    mapping['a']='A';mapping['c']='C';mapping['e']='E';
    assert(port111_basxlate(txt,5,mapping,256,dst,5,&count)==PORT109_APPROX);
    assert(count==3&&!memcmp(dst,"ACE",3));
    count=44;assert(port111_basxlate(txt,5,mapping,256,dst,2,&count)==PORT109_BAD_ARG&&count==44);
    uint32_t left[2]={0xffffffffu,0xffffffffu},right[2]={2,0},product[2]={0};
    assert(port111_cobmulq(left,right,product)==PORT109_OK&&product[0]==0xfffffffeu&&product[1]==0xffffffffu);
    assert(port111_cobmulq(left,right,left)==PORT109_OK&&left[0]==0xfffffffeu&&left[1]==0xffffffffu);
}
int main(void){clock_tests();complex_tests();realpow_tests();integer_tests();lun_tests();
    parse_tests();move_tests();basic_tests();
    puts("M111: 40 original RTL source modules, IA-32 freestanding subsets, edge tests PASS (guest integration UNVERIFIED)");
    return 0;
}
