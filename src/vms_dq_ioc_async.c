#include "vms_dq_ioc.h"

static void async_complete_callback(void *context,uint32_t status){
    (void)port_dq_ioc_async_complete(context,status);
}

/* DQ IOC dispatch adapter for a one-sector asynchronous hardware engine.
 * The IRP stays active until the caller drains its completion at IOC post
 * level; the hardware callback performs no user-memory or IOSB work. */
uint32_t port_dq_ioc_start_async_context(struct port_dq_ioc_queue *q,
    port_dq_ioc_async_start_fn start,void *start_context,uint32_t *tag){
    if(!q||!start||!tag||!q->read_sector)return PORT_DQ_IOC_BAD;
    uint32_t selected=0u;
    const uint32_t started=port_dq_ioc_start(q,&selected);
    if(started!=PORT_DQ_IOC_OK)return started;
    struct port_dq_ioc_packet *packet=&q->packets[q->active];
    if(packet->attempts==UINT8_MAX){
        packet->state=PORT_DQ_IOC_FAILED;
        return PORT_DQ_IOC_CORRUPT;
    }
    packet->attempts++;
    *tag=selected;
    const int accepted=start(start_context,packet->lbn,
        (uint16_t *)(void *)packet->sector,async_complete_callback,q);
    if(accepted==0)return PORT_DQ_IOC_OK;
    /* A rejecting backend guarantees it did not retain or call the callback. */
    if(packet->state!=PORT_DQ_IOC_ACTIVE||q->reads==UINT32_MAX)
        return PORT_DQ_IOC_CORRUPT;
    packet->state=PORT_DQ_IOC_FAILED;
    q->reads++;
    return PORT_DQ_IOC_IO;
}

uint32_t port_dq_ioc_start_async(struct port_dq_ioc_queue *q,
    port_dq_ioc_async_start_fn start,uint32_t *tag){
    return port_dq_ioc_start_async_context(q,start,q?q->context:0,tag);
}

/* IRQ-safe completion publication: the active request owns the only scratch
 * sector. Copy to the caller's buffer and retire the IRP later in IOC post. */
uint32_t port_dq_ioc_async_complete(struct port_dq_ioc_queue *q,
    uint32_t status){
    if(!q||q->active>=PORT_DQ_IOC_DEPTH||
       (status!=PORT_DQ_IOC_OK&&status!=PORT_DQ_IOC_IO)||
       q->reads==UINT32_MAX)return PORT_DQ_IOC_BAD;
    struct port_dq_ioc_packet *packet=&q->packets[q->active];
    if(packet->state!=PORT_DQ_IOC_ACTIVE)return PORT_DQ_IOC_BUSY;
    packet->state=status==PORT_DQ_IOC_OK?PORT_DQ_IOC_FINISHED:
                                             PORT_DQ_IOC_FAILED;
    q->reads++;
    return PORT_DQ_IOC_OK;
}
