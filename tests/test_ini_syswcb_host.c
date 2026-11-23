#include "vms_ini_syswcb.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#define CHECK(x) do { if(!(x)){fprintf(stderr,"FAIL %s:%d: %s\n",__FILE__,__LINE__,#x);return 1;} } while(0)
static void wr16(uint8_t *p,uint16_t x){p[0]=(uint8_t)x;p[1]=(uint8_t)(x>>8);}
static void wr32(uint8_t *p,uint32_t x){for(unsigned i=0;i<4;i++)p[i]=(uint8_t)(x>>(i*8));}
static void normalized(uint8_t *p,uint32_t count,uint32_t lbn){wr32(p,count);wr32(p+4u,lbn);}
int main(void){
    uint8_t diskmap[512],keep[512],tmpl[48];
    memset(diskmap,0xa5,sizeof diskmap);memset(tmpl,0xcc,sizeof tmpl);
    struct port_ini_syswcb_layout l={.header_length=sizeof tmpl,.size_word_offset=4,.map_count_word_offset=6};
    wr16(tmpl+l.size_word_offset,sizeof tmpl);wr16(tmpl+l.map_count_word_offset,0u);
    /* This 8+8-byte normalized representation is produced by SYSBOOT.
     * It is not the 6-byte real WCB representation used by the executive. */
    wr32(diskmap,24u);normalized(diskmap+4u,7u,128u);
    normalized(diskmap+12u,10u,140u);normalized(diskmap+20u,3u,160u);
    memcpy(keep,diskmap,sizeof keep);
    struct port_ini_syswcb wcb={0};
    CHECK(port_ini_syswcb_from_boot_map(diskmap,40u,tmpl,sizeof tmpl,&l,&wcb)==PORT_INI_SYSWCB_NOMEM);
    CHECK(!memcmp(diskmap,keep,sizeof keep));
    CHECK(port_ini_syswcb_from_boot_map(diskmap,sizeof diskmap,tmpl,sizeof tmpl,&l,&wcb)==PORT_INI_SYSWCB_OK);
    CHECK(wcb.record==diskmap&&wcb.retrieval_pointers==diskmap+sizeof tmpl);
    CHECK(wcb.mapping_count==3u&&wcb.record_size_field==66u&&wcb.record_length==66u);
    CHECK(diskmap[0]==0xccu&&diskmap[3]==0xccu&&diskmap[6]==3u&&diskmap[7]==0u);
    CHECK(diskmap[4]==66u&&diskmap[5]==0u);
    uint16_t blocks=0u;uint32_t lbn=0u;
    CHECK(port_ini_syswcb_retrieval(&wcb,0u,&blocks,&lbn)==PORT_INI_SYSWCB_OK && blocks==7u && lbn==128u);
    CHECK(port_ini_syswcb_retrieval(&wcb,1u,&blocks,&lbn)==PORT_INI_SYSWCB_OK && blocks==10u && lbn==140u);
    CHECK(port_ini_syswcb_retrieval(&wcb,2u,&blocks,&lbn)==PORT_INI_SYSWCB_OK && blocks==3u && lbn==160u);
    CHECK(port_ini_syswcb_retrieval(&wcb,3u,&blocks,&lbn)==PORT_INI_SYSWCB_BADPARAM);
    CHECK(diskmap[66u]==0xa5u && diskmap[511u]==0xa5u);

    /* Reject malformed maps without touching the caller's memory. */
    memcpy(diskmap,keep,sizeof keep);
    wr32(diskmap,7u);
    memcpy(keep,diskmap,sizeof keep);
    CHECK(port_ini_syswcb_from_boot_map(diskmap,sizeof diskmap,tmpl,sizeof tmpl,&l,&wcb)==PORT_INI_SYSWCB_BADMAP);
    CHECK(!memcmp(diskmap,keep,sizeof keep));
    wr32(diskmap,24u);wr32(diskmap+4u,65536u);
    CHECK(port_ini_syswcb_from_boot_map(diskmap,sizeof diskmap,tmpl,sizeof tmpl,&l,&wcb)==PORT_INI_SYSWCB_BADMAP);
    wr32(diskmap+4u,7u);wr32(diskmap+8u,UINT32_MAX-3u);
    CHECK(port_ini_syswcb_from_boot_map(diskmap,sizeof diskmap,tmpl,sizeof tmpl,&l,&wcb)==PORT_INI_SYSWCB_BADMAP);
    wr32(diskmap+8u,128u);wr32(diskmap+12u,0u);
    CHECK(port_ini_syswcb_from_boot_map(diskmap,sizeof diskmap,tmpl,sizeof tmpl,&l,&wcb)==PORT_INI_SYSWCB_BADMAP);
    wr32(diskmap+12u,10u);
    l.map_count_word_offset=5u;
    CHECK(port_ini_syswcb_from_boot_map(diskmap,sizeof diskmap,tmpl,sizeof tmpl,&l,&wcb)==PORT_INI_SYSWCB_BADPARAM);
    l.map_count_word_offset=6u;
    CHECK(port_ini_syswcb_from_boot_map(diskmap,sizeof diskmap,diskmap,48u,&l,&wcb)==PORT_INI_SYSWCB_BADPARAM);
    wr16(tmpl+l.size_word_offset,UINT16_MAX);
    CHECK(port_ini_syswcb_from_boot_map(diskmap,sizeof diskmap,tmpl,sizeof tmpl,&l,&wcb)==PORT_INI_SYSWCB_BADMAP);
    puts("INIT.MAR original normalized SYS.EXE map to 6-byte WCB records PASS");
    return 0;
}
