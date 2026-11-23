#include "vms_dq_crb.h"
#include "vms_v37_dyn_tags.h"

static void write16(uint8_t *p,uint16_t v){p[0]=(uint8_t)v;p[1]=(uint8_t)(v>>8);}
static void write32(uint8_t *p,uint32_t v){
    p[0]=(uint8_t)v;p[1]=(uint8_t)(v>>8);
    p[2]=(uint8_t)(v>>16);p[3]=(uint8_t)(v>>24);
}
static uint16_t read16(const uint8_t *p){
    return (uint16_t)((uint16_t)p[0]|((uint16_t)p[1]<<8));
}
static uint32_t read32(const uint8_t *p){
    return (uint32_t)p[0]|((uint32_t)p[1]<<8)|
           ((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);
}

uint32_t port_dq_crb_init(struct port_dq_crb *crb,
    port_dq_crb_handler handler,uint32_t vector){
    if(!crb||!handler||vector!=PORT_DQ_CRB_IRQ14_VECTOR||
       ((uintptr_t)crb&3u))return PORT_DQ_CRB_BAD;
    struct port_dq_crb next={0};
    const uint32_t address=(uint32_t)(uintptr_t)crb;
    /* Source CRB queue heads are initially self-linked.  The two source
     * VAX VEC dispatch instruction arrays stay zero on native IA-32. */
    write32(next.source,address);
    write32(next.source+4u,address);
    /* SYSDEF.MDL CRB: 16-byte header, 20 more bytes of links, then two
     * 36-byte VECs (DISPATCH, IDB, INITIAL and MAPREG fields). */
    write16(next.source+8u,PORT_DQ_CRB_STANDARD_BYTES);
    next.source[10u]=PORT_V37_DYN_CRB;
    write16(next.source+12u,1u);
    next.irq_handler=handler;next.vector=vector;next.ready=1u;
    *crb=next;
    return PORT_DQ_CRB_OK;
}
uint32_t port_dq_crb_validate(const struct port_dq_crb *crb){
    if(!crb||!crb->ready||crb->reserved[0]||crb->reserved[1]||
       crb->reserved[2]||!crb->irq_handler||
       crb->vector!=PORT_DQ_CRB_IRQ14_VECTOR||
       read32(crb->source)!=(uint32_t)(uintptr_t)crb||
       read32(crb->source+4u)!=(uint32_t)(uintptr_t)crb||
       read16(crb->source+8u)!=PORT_DQ_CRB_STANDARD_BYTES||
       crb->source[10u]!=PORT_V37_DYN_CRB||crb->source[11u]||
       read16(crb->source+12u)!=1u||crb->source[14u]||
       crb->source[15u])return PORT_DQ_CRB_BAD;
    for(unsigned i=15u;i<PORT_DQ_CRB_BYTES;i++)
        if(crb->source[i])return PORT_DQ_CRB_BAD;
    return PORT_DQ_CRB_OK;
}
uint32_t port_dq_crb_interrupt(struct port_dq_crb *crb,uint32_t vector){
    if(port_dq_crb_validate(crb)!=PORT_DQ_CRB_OK||vector!=crb->vector)
        return PORT_DQ_CRB_BAD;
    crb->interrupts++;
    crb->irq_handler();
    return PORT_DQ_CRB_OK;
}
