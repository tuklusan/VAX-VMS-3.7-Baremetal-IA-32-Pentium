#include "vms_iopost_end.h"
/* IOCIOPOST.MAR 149: VAX REMQUE fixes first, last and next.BLINK.
 * Clean released packet queue links, retaining its PID/status payload;
 * there is no actual lookaside-free operation at this early stage. */
uint32_t port_v37_iopost_pop(struct port_v37_iopost_queue *q,uint32_t *out){
    uint32_t address=0;const uint8_t *head=0;
    if(!out)return PORT_V37_IOPOST_BAD;
    const uint32_t ready=port_v37_iopost_peek(q,&address,&head);
    if(ready!=PORT_V37_IOPOST_OK)return ready;
    uint8_t *const packet=q->bytes+(address-q->first_irp);
    const uint32_t next=port_v37_read32(packet);
    if(q->queued==1u){
        q->first=q->sentinel;q->last=q->sentinel;
    }else{
        if(next==q->sentinel)return PORT_V37_IOPOST_CORRUPT;
        uint8_t *second=q->bytes+(next-q->first_irp);
        port_v37_write32(second+4u,q->sentinel);
        q->first=next;
    }
    --q->queued;
    port_v37_write32(packet,0u);
    port_v37_write32(packet+4u,0u);
    *out=address;
    return PORT_V37_IOPOST_OK;
}
