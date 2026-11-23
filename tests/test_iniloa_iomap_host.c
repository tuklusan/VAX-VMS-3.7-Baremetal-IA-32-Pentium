#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "vms_iniloa_iomap.h"
#define CHECK(x) do{if(!(x)){fprintf(stderr,"FAIL %s:%d %s\n",__FILE__,__LINE__,#x);return 1;}}while(0)
struct seen { uint32_t probes,maps;uint16_t last_port;int fail_map; };
static uint32_t probe(void *ctx,const struct port_ini_io_descriptor *a){
    struct seen *s=ctx;s->probes++;return a->io_base!=0x300u;
}
static uint32_t map(void *ctx,const struct port_ini_io_descriptor *a){
    struct seen *s=ctx;s->maps++;s->last_port=a->io_base;
    return s->fail_map?0u:1u;
}
int main(void){
    const struct port_ini_io_descriptor adapters[]={
       {0x3f8u,8u,1u,0u,4u},
       {0x1f0u,8u,2u,0u,14u},
       {0x300u,32u,3u,0u,10u}
    };
    struct seen s={0};struct port_ini_io_state state;
    memset(&state,0xf3,sizeof state);
    const struct port_ini_io_ops ops={&s,probe,map};
    CHECK(port_ini_iomap(adapters,3u,0x1f0u,&ops,&state)==PORT_INI_MAP_OK);
    CHECK(s.probes==3u && s.maps==2u&&s.last_port==0x1f0u);
    CHECK(state.num_nexus==2u);
    CHECK(state.confreg[0]==1u&&state.confreg[1]==2u);
    CHECK(state.sbiconf[0]==0x3f8u&&state.sbiconf[1]==0x1f0u);
    CHECK(state.boot.boot_adapter_number==1u);
    CHECK(state.boot.boot_adapter_virtual==0x1f0u);
    CHECK(state.boot.boot_csr_virtual==0x1f0u);
    CHECK(!state.sbiconf[2]&&!state.confreg[2]);
    struct port_ini_io_descriptor bad[]={
       {0x1f0u,8u,2u,0u,14u},
       {0x1f6u,8u,4u,0u,14u}
    };
    uint32_t mapped=s.maps;
    CHECK(port_ini_iomap(bad,2u,0x1f0u,&ops,&state)==PORT_INI_MAP_PORT_CONFLICT);
    CHECK(s.maps==mapped);
    CHECK(port_ini_iomap(adapters,3u,0x1f4u,&ops,&state)==PORT_INI_MAP_NO_BOOT_DEVICE);
    CHECK(port_ini_iomap(adapters,3u,0x300u,&ops,&state)==PORT_INI_MAP_NO_BOOT_DEVICE);
    CHECK(port_ini_iomap(adapters,PORT_INI_MAX_ADAPTERS+1u,0x1f0u,&ops,&state)==PORT_INI_MAP_BAD_ARGUMENT);
    s.fail_map=1;
    CHECK(port_ini_iomap(adapters,3u,0x1f0u,&ops,&state)==PORT_INI_MAP_IO_FAILURE);
    puts("INILOA INITADP I/O table and RPB tests PASS");return 0;
}
