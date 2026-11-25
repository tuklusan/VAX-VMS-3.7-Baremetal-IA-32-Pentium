#include <assert.h>
#include <stdint.h>
#include <string.h>
#include "vms_irp_queue.h"
#include "vms_v37_dyn_tags.h"

struct fixture { uint8_t irp[4][PORT_V37_IRP_MIN_BYTES]; };
static int resolve(void *context,uint32_t va,uint8_t **bytes,size_t *length){
    struct fixture *f=context;
    if(va<0x2000u||(va-0x2000u)%0x100u)return -1;
    const uint32_t slot=(va-0x2000u)/0x100u;
    if(slot>=4u)return -1;
    *bytes=f->irp[slot];*length=sizeof f->irp[slot];return 0;
}
static void init_irp(uint8_t *irp,uint8_t priority){
    memset(irp,0,PORT_V37_IRP_MIN_BYTES);
    irp[8]=(uint8_t)PORT_V37_IRP_MIN_BYTES;
    irp[10]=PORT_V37_DYN_IRP;irp[35]=priority;
}
static uint32_t read32(const uint8_t *p){
    return (uint32_t)p[0]|((uint32_t)p[1]<<8)|
        ((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);
}
int main(void){
    _Alignas(4) uint8_t ucb[132]={0};
    struct fixture f={0};
    struct port_v37_irp_queue q;
    assert(port_v37_irp_queue_init(&q,ucb,sizeof ucb,0x1000u,3u,
        resolve,&f)==PORT_V37_IRP_QUEUE_OK);
    init_irp(f.irp[0],1u);init_irp(f.irp[1],7u);init_irp(f.irp[2],7u);
    assert(port_v37_irp_queue_insert(&q,0x2000u)==PORT_V37_IRP_QUEUE_OK);
    assert(port_v37_irp_queue_insert(&q,0x2100u)==PORT_V37_IRP_QUEUE_OK);
    assert(port_v37_irp_queue_insert(&q,0x2200u)==PORT_V37_IRP_QUEUE_OK);
    uint32_t count=0u;
    assert(port_v37_irp_queue_validate(&q,&count)==PORT_V37_IRP_QUEUE_OK&&
        count==3u);
    const uint32_t head=0x1040u;
    assert(read32(ucb+64u)==0x2000u);
    assert(read32(f.irp[0])==0x2100u&&read32(f.irp[1])==0x2200u&&
        read32(f.irp[2])==head);
    f.irp[1][35]=0u;
    assert(port_v37_irp_queue_validate(&q,&count)==
        PORT_V37_IRP_QUEUE_CORRUPT);
    init_irp(f.irp[3],4u);
    assert(port_v37_irp_queue_insert(&q,0x2300u)==
        PORT_V37_IRP_QUEUE_CORRUPT);
    f.irp[1][35]=7u;
    assert(port_v37_irp_queue_insert(&q,0x2000u)==PORT_V37_IRP_QUEUE_BAD);
    assert(port_v37_irp_queue_insert(&q,0x2300u)==PORT_V37_IRP_QUEUE_FULL);
    assert(port_v37_irp_queue_remove(&q,0x2100u)==PORT_V37_IRP_QUEUE_OK);
    assert(read32(ucb+64u)==0x2000u&&read32(f.irp[0])==0x2200u&&
        read32(f.irp[2]+4u)==0x2000u);
    assert(port_v37_irp_queue_validate(&q,&count)==PORT_V37_IRP_QUEUE_OK&&
        count==2u);
    assert(port_v37_irp_queue_remove(&q,0x2100u)==
        PORT_V37_IRP_QUEUE_NOTFOUND);
    assert(port_v37_irp_queue_remove(&q,0x2200u)==PORT_V37_IRP_QUEUE_OK);
    assert(port_v37_irp_queue_remove(&q,0x2000u)==PORT_V37_IRP_QUEUE_OK);
    assert(port_v37_irp_queue_validate(&q,&count)==PORT_V37_IRP_QUEUE_OK&&
        count==0u);
    assert(port_v37_irp_queue_insert(&q,0x2000u)==PORT_V37_IRP_QUEUE_OK);
    assert(port_v37_irp_queue_insert(&q,0x2100u)==PORT_V37_IRP_QUEUE_OK);
    assert(port_v37_irp_queue_insert(&q,0x2200u)==PORT_V37_IRP_QUEUE_OK);
    assert(port_v37_irp_queue_insert(&q,0x2300u)==PORT_V37_IRP_QUEUE_FULL);
    f.irp[0][0]^=1u;
    assert(port_v37_irp_queue_validate(&q,0)==PORT_V37_IRP_QUEUE_CORRUPT);
    return 0;
}
