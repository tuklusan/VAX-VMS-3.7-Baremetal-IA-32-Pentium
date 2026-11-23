#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "vms_qio_irp_pool.h"
#include "vms_qio_post.h"
#include "vms_v37_dyn_tags.h"
#define CHECK(x) do{if(!(x)){fprintf(stderr,"irp pool FAIL %d: %s\n",__LINE__,#x);return 1;}}while(0)
struct sync_state {uint32_t current,enters,exits;};
static uint32_t raise_sync(void *arg){
    struct sync_state *s=arg;uint32_t old=s->current;s->current=4u;++s->enters;return old;
}
static void put16(uint8_t *p,uint16_t v){p[0]=(uint8_t)v;p[1]=(uint8_t)(v>>8);}
static void restore_ipl(void *arg,uint32_t old){
    struct sync_state *s=arg;if(s->current!=4u)s->current=UINT32_MAX;
    s->current=old;++s->exits;
}
int main(void){
    enum {BASE=0x326000u,SENTINEL=0x328000u,STRIDE=144u};
    uint8_t memory[512u];memset(memory,0xa5,sizeof memory);
    struct port_ini_packet_area area={.memory=memory,.address=BASE,
        .available_bytes=sizeof memory,.count=3u,.packet_bytes=STRIDE,
        .minimum_bytes=STRIDE,.sentinel_address=SENTINEL};
    struct port_ini_packet_list list={0};
    CHECK(port_ini_packet_init(&area,&list)==PORT_INI_PACKETS_OK);
    struct sync_state state={.current=1u};
    const struct port_v37_irp_pool_sync sync={&state,raise_sync,restore_ipl};
    uint8_t *p[3]={0};uint32_t va[3]={0};
    for(unsigned i=0;i<3u;i++){
        CHECK(port_v37_irp_pool_alloc(&list,PORT_V37_IRP_SIZE,&sync,
                                      &p[i],&va[i])==PORT_V37_IRP_POOL_OK);
        CHECK(va[i]==BASE+(2u-i)*STRIDE&&p[i]==memory+(2u-i)*STRIDE);
        CHECK(p[i][10u]==PORT_V37_DYN_IRP&&p[i][11u]==0u);
        CHECK((uint16_t)(p[i][8u]|((uint16_t)p[i][9u]<<8))==STRIDE);
        CHECK(state.current==1u&&state.enters==i+1u&&state.exits==i+1u);
    }
    uint8_t *none=0;uint32_t no_va=0;
    CHECK(port_v37_irp_pool_alloc(&list,PORT_V37_IRP_SIZE,&sync,
          &none,&no_va)==PORT_V37_IRP_POOL_EMPTY);
    CHECK(!none&&!no_va&&state.current==1u&&state.enters==4u&&state.exits==4u);
    put16(p[1]+8u,PORT_V37_IRP_SIZE);
    CHECK(port_v37_irp_pool_free(&list,p[1],va[1],PORT_V37_IRP_SIZE,&sync)==PORT_V37_IRP_POOL_OK);
    CHECK(list.free_count==1u&&list.next==va[1]&&list.previous==va[1]);
    CHECK(port_v37_irp_pool_free(&list,p[1],va[1],PORT_V37_IRP_SIZE,&sync)==PORT_V37_IRP_POOL_DUP);
    CHECK(port_v37_irp_pool_alloc(&list,PORT_V37_IRP_SIZE,&sync,
          &none,&no_va)==PORT_V37_IRP_POOL_OK&&none==p[1]&&no_va==va[1]);
    put16(p[0]+8u,PORT_V37_IRP_SIZE);
    put16(p[2]+8u,PORT_V37_IRP_SIZE);
    put16(p[1]+8u,PORT_V37_IRP_SIZE);
    CHECK(port_v37_irp_pool_free(&list,p[0],va[0],PORT_V37_IRP_SIZE,&sync)==PORT_V37_IRP_POOL_OK);
    CHECK(port_v37_irp_pool_free(&list,p[2],va[2],PORT_V37_IRP_SIZE,&sync)==PORT_V37_IRP_POOL_OK);
    CHECK(port_v37_irp_pool_free(&list,p[1],va[1],PORT_V37_IRP_SIZE,&sync)==PORT_V37_IRP_POOL_OK);
    CHECK(list.free_count==3u&&list.next==va[0]&&list.previous==va[1]);
    CHECK(state.current==1u&&state.enters==state.exits);
    uint32_t saved=memory[4u];memory[4u]^=1u;
    CHECK(port_v37_irp_pool_alloc(&list,PORT_V37_IRP_SIZE,&sync,
          &none,&no_va)==PORT_V37_IRP_POOL_CORRUPT);
    memory[4u]=(uint8_t)saved;
    CHECK(port_v37_irp_pool_alloc(&list,145u,&sync,
          &none,&no_va)==PORT_V37_IRP_POOL_BAD);
    puts("MEMORYALC.MAR IRP lookaside allocate/return, FIFO, IPL and exhaustion PASS");
    return 0;
}
