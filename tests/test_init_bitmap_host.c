#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "vms_init_bitmap.h"
#include "vms_f11_home.h"
static int failed;
#define CHECK(x) do {if(!(x)){fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x);failed++;}}while(0)
struct storage {uint8_t sectors[5][512];uint32_t last_lbn;unsigned writes;int deny;};
static uint32_t writer(void *ctx,uint32_t lbn,const uint8_t b[512]){
    struct storage *s=ctx;
    if(lbn<100u||lbn>=105u){failed++;return 2u;}
    memcpy(s->sectors[lbn-100u],b,512);
    s->last_lbn=lbn;s->writes++;
    return s->deny?2u:1u;
}
static uint16_t rd16(const uint8_t *b){return (uint16_t)(b[0]|(b[1]<<8));}
int main(void){
    struct storage st={0};uint8_t b[512];
    const struct port_init_allocation a[]={
        {12288,3}, {0,3}, {900,3}, {10,3}, {18,6}
    };
    const struct port_init_bitmap_options o={0,100,5,16384,3,63,16,16};
    CHECK(port_init_bitmap(&o,a,5,b,writer,&st)==1u);
    CHECK(st.writes==5u && st.last_lbn==104u);
    CHECK(rd16(st.sectors[0])==513u&&rd16(st.sectors[0]+2)==3u);
    CHECK(rd16(st.sectors[0]+510)!=0u);
    CHECK(!(st.sectors[1][0]&1u));
    CHECK(!(st.sectors[1][0]&(1u<<3u)));
    CHECK(!(st.sectors[1][0]&(1u<<6u)));
    CHECK(!(st.sectors[1][0]&(1u<<7u)));
    CHECK(!(st.sectors[1][300u/8u]&(1u<<(300u&7u))));
    CHECK(!(st.sectors[2][0]&1u));
    CHECK(st.sectors[3][0]==0u&&st.sectors[4][0]==0u);
    uint8_t copy[512];memcpy(copy,st.sectors[0],512);
    CHECK(port_f11_checksum2(copy,510u));
    struct storage old={0};
    struct port_init_bitmap_options level1=o;level1.structure1=1u;
    CHECK(port_init_bitmap(&level1,a,5,b,writer,&old)==1u);
    CHECK(old.sectors[0][3]==4u);
    CHECK(rd16(old.sectors[0]+4)==4096u);
    CHECK(port_init_bitmap(0,a,5,b,writer,&old)==0u);
    if(failed)return 1;
    puts("PASS: INIT_BITMAP ODS-1 and ODS-2 control blocks, used/free allocation bits");
    return 0;
}
