#include "vms_qio_post.h"
/* INSQUE IRP,@IOC$GL_PSBL appends after current tail. A corrupted
 * queue never receives an extra link and a packet cannot be posted twice. */
uint32_t port_v37_post_append(struct port_v37_iopost_queue *q,uint32_t address){
    uint32_t valid=port_v37_post_validate(q);
    if(valid!=PORT_V37_QIO_OK)return valid;
    if(address<q->first_irp || (address-q->first_irp)%q->packet_stride ||
       (address-q->first_irp)/q->packet_stride>=q->packet_capacity)
        return PORT_V37_QIO_BAD;
    if(q->queued==q->packet_capacity)return PORT_V37_QIO_FULL;
    for(uint32_t i=0,cursor=q->first;i<q->queued;i++){
        if(cursor==address)return PORT_V37_QIO_DUP;
        cursor=port_v37_read32(q->bytes+(cursor-q->first_irp));
    }
    uint8_t *packet=q->bytes+(address-q->first_irp);
    port_v37_write32(packet,q->sentinel);
    port_v37_write32(packet+4u,q->last);
    if(q->queued){
        uint8_t *tail=q->bytes+(q->last-q->first_irp);
        port_v37_write32(tail,address);
    }else q->first=address;
    q->last=address;++q->queued;
    return PORT_V37_QIO_OK;
}
