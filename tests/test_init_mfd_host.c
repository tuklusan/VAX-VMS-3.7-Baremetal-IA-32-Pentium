#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "vms_init_mfd.h"

static int failed;
#define CHECK(x) do {if(!(x)){fprintf(stderr,"FAIL %u: %s\n",(unsigned)__LINE__,#x);failed++;}}while(0)
struct sink { uint8_t block[512]; uint32_t last_lbn; unsigned calls; int deny; };
static uint32_t write_block(void *ctx,uint32_t lbn,const uint8_t block[512]) {
    struct sink *s=ctx;
    memcpy(s->block,block,512);
    s->last_lbn=lbn;
    s->calls++;
    return s->deny?PORT_INIT_MFD_WRITE_FAILED:PORT_INIT_MFD_NORMAL;
}
static uint16_t read16(const uint8_t *p) {return (uint16_t)(p[0]|(p[1]<<8));}
static uint16_t encode_rad50(const char *p){
    const char *a=" ABCDEFGHIJKLMNOPQRSTUVWXYZ$.%0123456789";
    uint16_t v=0;
    for(unsigned i=0;i<3;i++){
        const char *c=strchr(a,p[i]);
        if(!c){failed++;return 0;}
        v=(uint16_t)(v*40u+(unsigned)(c-a));
    }
    return v;
}
int main(void) {
    uint8_t b[512];struct sink s={{0},0,0,0};
    CHECK(port_init_mfd(12,0,b,write_block,&s)==PORT_INIT_MFD_NORMAL);
    CHECK(s.calls==1u&&s.last_lbn==12u&&memcmp(b,s.block,512)==0);
    const char *names[]={"000000.DIR","BACKUP.SYS","BADBLK.SYS",
       "BADLOG.SYS","BITMAP.SYS","CONTIN.SYS","CORIMG.SYS",
       "INDEXF.SYS","VOLSET.SYS"};
    const uint16_t ids[]={4,8,3,9,2,7,5,1,6};
    for(unsigned i=0;i<9u;i++){
        uint8_t *p=b+i*24u;
        CHECK(read16(p)==22u&&read16(p+2u)==1u);
        CHECK(p[4]==0&&p[5]==10);
        CHECK(!memcmp(p+6,names[i],10));
        CHECK(read16(p+16u)==1u&&read16(p+18u)==ids[i]&&
              read16(p+20u)==ids[i]&&read16(p+22u)==0u);
    }
    CHECK(read16(b+216u)==0xffffu);
    for(unsigned i=218;i<512u;i++)CHECK(b[i]==0u);
    memset(b,0xa5,sizeof b);
    CHECK(port_init_mfd(23,1,b,write_block,&s)==PORT_INIT_MFD_NORMAL);
    CHECK(s.calls==2u&&s.last_lbn==23u);
    const char *old_names[]={"INDEXF   SYS","BITMAP   SYS","BADBLK   SYS",
                             "000000   DIR","CORIMG   SYS"};
    for(unsigned i=0;i<5u;i++){
        const uint8_t *p=b+16u*i;
        CHECK(read16(p)==i+1u&&read16(p+2u)==i+1u&&read16(p+4u)==0u);
        for(unsigned j=0;j<4u;j++)CHECK(read16(p+6u+j*2u)==encode_rad50(old_names[i]+j*3u));
        CHECK(read16(p+14u)==1u);
    }
    for(unsigned i=80;i<512u;i++)CHECK(b[i]==0u);
    s.deny=1;
    CHECK(port_init_mfd(14,0,b,write_block,&s)==PORT_INIT_MFD_WRITE_FAILED);
    CHECK(port_init_mfd(14,0,0,write_block,&s)==PORT_INIT_MFD_BAD_ARGUMENT);
    CHECK(port_init_mfd(14,0,b,0,&s)==PORT_INIT_MFD_BAD_ARGUMENT);
    CHECK(s.calls==3u);
    if(failed)return 1;
    puts("PASS: source INIT_MFD ODS-1 RAD50 and ODS-2 directory records, disk write");
    return 0;
}
