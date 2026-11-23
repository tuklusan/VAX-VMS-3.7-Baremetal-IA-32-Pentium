#include "vms_ddtreloc.h"
#include <stdio.h>
#include <string.h>
#define CHECK(c) do{if(!(c)){fprintf(stderr,"DDTRELOC line %d: %s\n",__LINE__,#c);return 1;}}while(0)
static void base_image(uint8_t *b,size_t n){
    memset(b,0,n);
    const unsigned offsets[]={0u,4u,8u,12u,16u,24u,28u,32u};
    for(unsigned i=0;i<sizeof offsets/sizeof offsets[0];i++)
        port_ddtreloc_write32(b+offsets[i],0x100u+i*4u);
    port_ddtreloc_write32(b+8u,40u);
    b[36u]=136u;b[37u]=0;
    for(unsigned i=0;i<10u;i++)
        port_ddtreloc_write32(b+40u+24u+12u*i,0x100u+i);
}
int main(void){
    uint8_t buf[176];base_image(buf,sizeof buf);
    struct port_ddtreloc_image im={buf,sizeof buf,0x80010000u};
    uint32_t off=0;uint16_t len=0;
    CHECK(port_ddtreloc_preflight(&im,&off,&len)==PORT_DDTRELOC_OK);
    CHECK(off==40u&&len==136u);
    CHECK(port_ddtreloc_apply(&im)==PORT_DDTRELOC_OK);
    CHECK(port_ddtreloc_read32(buf+8u)==0x80010028u);
    CHECK(port_ddtreloc_read32(buf)==0x80010100u);
    CHECK(port_ddtreloc_read32(buf+40u+24u)==0x80010038u+0x100u);
    CHECK(port_ddtreloc_read32(buf+40u+24u+12u*9u)==0x80010038u+12u*9u+0x109u);
    /* System pointers with sign bit set are never adjusted again. */
    uint8_t snapshot[sizeof buf];memcpy(snapshot,buf,sizeof buf);
    CHECK(port_ddtreloc_apply(&im)==PORT_DDTRELOC_OK);
    CHECK(!memcmp(snapshot,buf,sizeof buf));
    /* Each malformed image fails before a write. */
    base_image(buf,sizeof buf);
    port_ddtreloc_write32(buf+8u,176u);
    memcpy(snapshot,buf,sizeof buf);
    CHECK(port_ddtreloc_apply(&im)==PORT_DDTRELOC_UNBOUND);
    CHECK(!memcmp(snapshot,buf,sizeof buf));
    base_image(buf,sizeof buf);
    buf[36u]=135u;memcpy(snapshot,buf,sizeof buf);
    CHECK(port_ddtreloc_apply(&im)==PORT_DDTRELOC_UNBOUND);
    CHECK(!memcmp(snapshot,buf,sizeof buf));
    base_image(buf,sizeof buf);
    im.pc_base=UINT32_MAX-1u;
    CHECK(port_ddtreloc_apply(&im)==PORT_DDTRELOC_BAD);
    im.pc_base=0x80010000u;
    im.length=38u;
    CHECK(port_ddtreloc_apply(&im)==PORT_DDTRELOC_UNBOUND);
    puts("IOC$RELOC_DDT private DDT/FDT source relocation PASS; no DQA0 binding");
    return 0;
}
