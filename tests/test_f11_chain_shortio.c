#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "vms_f11_chain.h"
#define CHECK(x) do{if(!(x)){fprintf(stderr,"FH2 partial physical IO %d: %s\n",__LINE__,#x);return 1;}}while(0)
struct device{unsigned reads,fail_at;};
static uint32_t read_sector(void *opaque,uint32_t lbn,uint8_t b[512]){
    struct device *d=opaque;
    if(++d->reads==d->fail_at)return PORT_DQ_DIR_MAP;
    if(lbn!=700u&&lbn!=1234u)return PORT_DQ_DIR_MAP;
    for(unsigned i=0;i<512u;i++)b[i]=(uint8_t)(lbn+i);
    return PORT_DQ_DIR_OK;
}
int main(void){
    struct port_f11_chain chain={0};
    chain.count=2u;chain.blocks=2u;
    chain.extents[0]=(struct port_f11_extent){1u,700u};
    chain.extents[1]=(struct port_f11_extent){1u,1234u};
    struct device io={0u,2u};
    struct port_f11_boot_volume vol={0};
    vol.read_sector=read_sector;vol.context=&io;
    uint8_t result[700];memset(result,0xa5,sizeof result);
    uint32_t done=UINT32_MAX;
    CHECK(port_f11_chain_read(&vol,&chain,400u,600u,result,
         sizeof result,&done)==PORT_F11_CHAIN_IO);
    CHECK(done==112u&&io.reads==2u);
    for(unsigned i=0;i<112u;i++)CHECK(result[i]==(uint8_t)(700u+400u+i));
    for(unsigned i=112u;i<sizeof result;i++)CHECK(result[i]==0xa5u);
    io.reads=0u;io.fail_at=1u;done=UINT32_MAX;
    CHECK(port_f11_chain_read(&vol,&chain,400u,600u,result,
         sizeof result,&done)==PORT_F11_CHAIN_IO);
    CHECK(done==0u&&io.reads==1u);
    io.reads=0u;io.fail_at=0u;
    CHECK(port_f11_chain_read(&vol,&chain,400u,600u,result,
         sizeof result,&done)==PORT_F11_CHAIN_OK);
    CHECK(done==600u&&io.reads==2u);
    for(unsigned i=0;i<600u;i++){
        unsigned off=400u+i;
        CHECK(result[i]==(uint8_t)((off<512u?700u:1234u)+(off%512u)));
    }
    done=UINT32_MAX;
    CHECK(port_f11_chain_read(&vol,&chain,400u,625u,result,
         sizeof result,&done)==PORT_F11_CHAIN_MAP);
    CHECK(done==UINT32_MAX);
    CHECK(port_f11_chain_read(&vol,&chain,UINT32_MAX,1u,result,
         sizeof result,&done)==PORT_F11_CHAIN_BAD);
    CHECK(port_f11_chain_read(&vol,&chain,0u,701u,result,
         sizeof result,&done)==PORT_F11_CHAIN_BAD);
    CHECK(port_f11_chain_read(&vol,&chain,0u,0u,result,
         sizeof result,&done)==PORT_F11_CHAIN_OK&&done==0u);
    puts("FIL$READVBN extension cross-sector partial I/O and no false success PASS");
    return 0;
}
