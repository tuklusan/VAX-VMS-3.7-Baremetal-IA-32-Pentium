#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "vms_rtl85.h"
static struct port_rtl_desc desc(const char *s){
    struct port_rtl_desc d={0,0,PORT_RTL_CLASS_S,(const uint8_t *)s,0};
    while(s[d.length])d.length++;
    return d;
}
static void test_desc(void){
    struct port_rtl_desc a=desc("HELLO WORLD"),b=desc("WORLD");
    struct port_rtl_view v;
    assert(port_lib_analyze_sdesc(&a,&v)==PORT_RTL_NORMAL);
    assert(v.length==11 && v.data!=0);
    assert(port_lib_len(&a)==11 && port_lib_ichar(&a)=='H');
    assert(port_lib_index(&a,&b)==7 && port_lib_matchc(&b,&a)==7);
    assert(port_lib_index(&a,&a)==1 && port_lib_matchc(&a,&a)==1);
    b=desc("");assert(port_lib_index(&a,&b)==1 && port_lib_matchc(&b,&a)==1);
    a=desc("");assert(port_lib_matchc(&b,&a)==1);
    b=desc("A");assert(port_lib_index(&a,&b)==0);
    a.class_code=PORT_RTL_CLASS_A;a.array_size=100;
    assert(port_lib_analyze_sdesc(&a,&v)==PORT_RTL_NORMAL && v.length==100);
    a=desc("A");a.class_code=PORT_RTL_CLASS_A;a.array_size=65535u;
    assert(port_lib_analyze_sdesc(&a,&v)==PORT_RTL_NORMAL && v.length==65535u);
    a.array_size=65536u;assert(port_lib_analyze_sdesc(&a,&v)==PORT_RTL_INVSTRDES);
    a.class_code=PORT_RTL_CLASS_NCA;a.array_size=2;
    assert(port_lib_analyze_sdesc(&a,&v)==PORT_RTL_NORMAL && v.length==2);
    a.class_code=PORT_RTL_CLASS_SD;a.length=1;
    assert(port_lib_analyze_sdesc(&a,&v)==PORT_RTL_NORMAL && v.length==1);
    uint8_t vs[]={3,0,'a','b','c'};
    a.class_code=PORT_RTL_CLASS_VS;a.pointer=vs;
    assert(port_lib_analyze_sdesc(&a,&v)==PORT_RTL_NORMAL && v.length==3 && v.data==vs+2);
    assert(port_lib_len(&a)==3 && port_lib_ichar(&a)=='a');
    a.class_code=5;assert(port_lib_analyze_sdesc(&a,&v)==PORT_RTL_INVSTRDES);
    a.class_code=PORT_RTL_CLASS_S;a.length=0;a.pointer=0;
    assert(port_lib_analyze_sdesc(&a,&v)==PORT_RTL_NORMAL && port_lib_ichar(&a)==0);
    a.length=1;assert(port_lib_analyze_sdesc(&a,&v)==PORT_RTL_INVSTRDES);
}
static void test_strings(void){
    struct port_rtl_desc s=desc("AAABCD"),a=desc("A"),b=desc("B");
    assert(port_lib_skpc(&a,&s)==4);
    assert(port_lib_locc(&b,&s)==4);
    assert(port_lib_locc(&a,&s)==1);
    assert(port_lib_skpc(&b,&s)==1);
    struct port_rtl_desc empty=desc("");
    assert(port_lib_skpc(&b,&empty)==0 && port_lib_locc(&b,&empty)==0);
    uint8_t tbl[256]={0};tbl['A']=4;tbl['C']=2;
    assert(port_lib_scanc(&s,tbl,2)==5);
    assert(port_lib_spanc(&s,tbl,4)==4);
    assert(port_lib_scanc(&s,tbl,8)==0);
    assert(port_lib_spanc(&empty,tbl,1)==0);
    assert(port_lib_matchc(&b,&s)==4 && port_lib_index(&s,&b)==4);
    char buf[16]="0123456789";
    assert(port_lib_movc3(6,buf,buf+2)==1);
    assert(memcmp(buf,"0101234589",10)==0);
    assert(port_lib_movc3(7,buf+2,buf)==1);
    assert(memcmp(buf,"0123458589",10)==0);
    assert(port_lib_movc5(3,buf,'_',8,buf+2)==1);
    assert(memcmp(buf,"01012_____",10)==0);
    assert(port_lib_movc5(8,buf,'X',2,buf)==1);
    assert(port_lib_movc5(0,0,'!',0,0)==1);
}
static void test_bits(void){
    uint8_t b[12]={0};
    uint32_t out=123;
    assert(port_lib_insv(0xABCDu,7,16,b)==PORT_RTL_NORMAL);
    assert(port_lib_extzv(7,16,b,&out)==PORT_RTL_NORMAL && out==0xABCDu);
    assert(port_lib_extzv(7,0,b,&out)==PORT_RTL_NORMAL && !out);
    assert(port_lib_extzv(7,33,b,&out)==PORT_RTL_RESERVED_OPERAND);
    assert(port_lib_insv(0,7,33,b)==PORT_RTL_RESERVED_OPERAND);
    int32_t sx=0;
    assert(port_lib_insv(0xFu,12,4,b)==PORT_RTL_NORMAL);
    assert(port_lib_extv(12,4,b,&sx)==PORT_RTL_NORMAL && sx==-1);
    assert(port_lib_insv(0x87654321u,32,32,b)==PORT_RTL_NORMAL);
    assert(port_lib_extzv(32,32,b,&out)==PORT_RTL_NORMAL && out==0x87654321u);
    assert(port_lib_extv(32,32,b,&sx)==PORT_RTL_NORMAL && (uint32_t)sx==0x87654321u);
    assert(port_lib_ffs(0,32,b,&out)==PORT_RTL_NORMAL && out==7);
    assert(port_lib_ffc(7,8,b,&out)==PORT_RTL_NORMAL);
    assert(port_lib_ffs(90,0,b,&out)==PORT_RTL_NOTFOU);
    assert(port_lib_ffs(0,33,b,&out)==PORT_RTL_RESERVED_OPERAND);
    assert(port_lib_ffc(0,33,b,&out)==PORT_RTL_RESERVED_OPERAND);
    uint8_t minus[3]={0};
    assert(port_lib_insv(7,3,3,minus)==1);
    assert(port_lib_extzv(3,3,minus,&out)==1 && out==7);
}
static void test_tables(void){
    assert(port_lib_ab_upcase['a']=='A' && port_lib_ab_upcase['z']=='Z');
    assert(port_lib_ab_upcase['Z']=='Z' && port_lib_ab_upcase[255]==255);
    assert(port_lib_ab_cvt_u_o[0]=='{' && port_lib_ab_cvt_u_o[19]=='R');
    assert(port_lib_ab_cvt_o_u['}']=='0' && port_lib_ab_cvt_o_u['J']=='1');
    assert(port_lib_ab_cvt_o_u['R']=='9' && port_lib_ab_cvt_o_u['!']=='0');
    assert(port_lib_ab_cvtpt_u[0x9C]=='9' && port_lib_ab_cvtpt_u[0x09]==0);
    assert(port_lib_ab_cvttp_u['9']==0x9C && port_lib_ab_cvttp_u['x']==0);
    assert(port_lib_ab_cvtpt_o[0x9C]=='I' && port_lib_ab_cvtpt_o[0x9D]=='R');
    assert(port_lib_ab_cvttp_o['A']==0x1C && port_lib_ab_cvttp_o['R']==0x9D);
    assert(port_lib_ab_cvtpt_z[0x9D]==0x79 && port_lib_ab_cvtpt_z[0x9C]=='9');
    assert(port_lib_ab_cvttp_z['9']==0x9C && port_lib_ab_cvttp_z['y']==0x9D);
}
static void test_radix(void){
    uint32_t v=42;
    assert(port_sys_cvt_dtb(3,(const uint8_t *)"123",&v)==1 && v==123);
    assert(port_sys_cvt_otb(3,(const uint8_t *)"177",&v)==1 && v==127);
    assert(port_sys_cvt_htb(4,(const uint8_t *)"aBCf",&v)==1 && v==0xABCF);
    assert(port_sys_cvt_htb(8,(const uint8_t *)"FFFFFFFF",&v)==1 && v==UINT32_MAX);
    assert(port_sys_cvt_htb(9,(const uint8_t *)"1FFFFFFFF",&v)==0);
    assert(port_sys_cvt_dtb(11,(const uint8_t *)"-4294967295",&v)==1 && v==1);
    assert(port_sys_cvt_dtb(10,(const uint8_t *)"4294967296",&v)==0);
    assert(port_sys_cvt_otb(1,(const uint8_t *)"8",&v)==0);
    assert(port_sys_cvt_dtb(0,(const uint8_t *)"",&v)==0);
}
/* This batch rechecks all 25 original RTL/SRC/LIB*.MAR equivalents.
 * These seven FNV-1a constants were calculated directly from all 256
 * source .BYTE values at the pinned DEC revision. Only checksums, not
 * any copy of the original DEC source files, are retained here. */
static uint32_t rtl_fnv32(const uint8_t *p,unsigned count){
    uint32_t h=UINT32_C(2166136261);
    for(unsigned i=0;i<count;i++)h=(h^p[i])*UINT32_C(16777619);
    return h;
}
static void test_original_rtl_tables_all_bytes(void){
    assert(rtl_fnv32(port_lib_ab_cvt_o_u,256u)==UINT32_C(0xc5ce4a2a));
    assert(rtl_fnv32(port_lib_ab_cvtpt_u,256u)==UINT32_C(0xb39a9e43));
    assert(rtl_fnv32(port_lib_ab_cvttp_u,256u)==UINT32_C(0x53d9ee6d));
    assert(rtl_fnv32(port_lib_ab_cvtpt_o,256u)==UINT32_C(0x5f39f845));
    assert(rtl_fnv32(port_lib_ab_cvttp_o,256u)==UINT32_C(0x179248de));
    assert(rtl_fnv32(port_lib_ab_cvtpt_z,256u)==UINT32_C(0x4ce08143));
    assert(rtl_fnv32(port_lib_ab_cvttp_z,256u)==UINT32_C(0x54bd2327));
    const char overpunch[]="{ABCDEFGHI}JKLMNOPQR";
    for(unsigned i=0;i<sizeof overpunch-1u;i++)
        assert(port_lib_ab_cvt_u_o[i]==(uint8_t)overpunch[i]);
    for(unsigned i=0;i<256u;i++){
        unsigned expected=i;
        if(i>='a' && i<='z')expected-=('a'-'A');
        assert(port_lib_ab_upcase[i]==expected);
    }
}
static void test_rtl_16_functions_boundary_cases(void){
    uint8_t bits[12]={0};uint32_t value=UINT32_C(0xFFFFFFFF);
    int32_t sx=0;
    assert(port_lib_insv(UINT32_C(0x8fffffff),8,32,bits)==PORT_RTL_NORMAL);
    assert(port_lib_extzv(8,32,bits,&value)==PORT_RTL_NORMAL &&
           value==UINT32_C(0x8fffffff));
    assert(port_lib_extv(8,32,bits,&sx)==PORT_RTL_NORMAL &&
           (uint32_t)sx==UINT32_C(0x8fffffff));
    assert(port_lib_ffs(8,32,bits,&value)==PORT_RTL_NORMAL && value==8u);
    assert(port_lib_ffc(8,32,bits,&value)==PORT_RTL_NORMAL && value==36u);
    assert(port_lib_ffc(8,0,bits,0)==PORT_RTL_NOTFOU);
    assert(port_lib_ffs(8,0,bits,0)==PORT_RTL_NOTFOU);
    assert(port_lib_extzv(0,32,0,&value)==PORT_RTL_INVSTRDES);
    assert(port_lib_insv(1,0,32,0)==PORT_RTL_INVSTRDES);
    assert(port_lib_ffs(0,8,0,&value)==PORT_RTL_INVSTRDES);
    const char *literal="ABABABCD";
    struct port_rtl_desc longstr=desc(literal),pattern=desc("ABCD");
    assert(port_lib_index(&longstr,&pattern)==5u);
    assert(port_lib_matchc(&pattern,&longstr)==5u);
    struct port_rtl_desc zero=desc("");
    assert(port_lib_index(&longstr,&zero)==1u);
    assert(port_lib_len(&longstr)==8u && port_lib_ichar(&longstr)=='A');
    assert(port_lib_locc(&pattern,&longstr)==1u);
    assert(port_lib_skpc(&pattern,&longstr)==2u);
    uint8_t all[256]={0};all['A']=0x40u;all['B']=0x80u;
    assert(port_lib_scanc(&longstr,all,0x80u)==2u);
    assert(port_lib_spanc(&longstr,all,0x40u)==2u);
    char overlap[16]="abcdefghi";
    assert(port_lib_movc3(7,overlap,overlap+2)==PORT_RTL_NORMAL);
    assert(memcmp(overlap,"ababcdefg",9)==0);
    assert(port_lib_movc5(4,overlap,'X',8,overlap+1)==PORT_RTL_NORMAL);
    assert(memcmp(overlap,"aababXXXX",9)==0);
}
int main(void){
    test_desc();test_strings();test_bits();test_tables();test_radix();
    test_original_rtl_tables_all_bytes();test_rtl_16_functions_boundary_cases();
    puts("VMS 3.7 RTL 25 original source modules: full-byte tables and directed routines PASS");
    return 0;
}
