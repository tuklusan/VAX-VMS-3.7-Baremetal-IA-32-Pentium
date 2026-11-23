#include "vms_dq_ioc_write.h"
uint32_t port_dq_ioc_write_init(struct port_dq_ioc_write_queue *q,
    port_dq_ioc_write_backend backend,void *context){
    if(!q||!backend)return PORT_DQ_IOCW_BAD;
    *q=(struct port_dq_ioc_write_queue){0};
    q->backend=backend;q->context=context;
    return PORT_DQ_IOCW_OK;
}
uint32_t port_dq_ioc_write_prepare(struct port_dq_ioc_write_queue *q,
    uint32_t tag,uint32_t lbn,const uint8_t *payload,size_t size){
    if(!q||!q->backend||!tag||!payload||size<512u||lbn>=0x10000000u||
       q->order==UINT32_MAX)return PORT_DQ_IOCW_BAD;
    unsigned slot=PORT_DQ_IOCW_DEPTH;
    for(unsigned i=0;i<PORT_DQ_IOCW_DEPTH;i++){
        const struct port_dq_ioc_write_packet *p=&q->packets[i];
        if(p->state==0u){if(slot==PORT_DQ_IOCW_DEPTH)slot=i;}
        else if(p->tag==tag)return PORT_DQ_IOCW_FULL;
    }
    if(slot==PORT_DQ_IOCW_DEPTH)return PORT_DQ_IOCW_FULL;
    struct port_dq_ioc_write_packet *p=&q->packets[slot];
    *p=(struct port_dq_ioc_write_packet){0};
    p->tag=tag;p->lbn=lbn;p->order=++q->order;p->state=PORT_DQ_IOCW_PREPARED;
    for(unsigned i=0;i<512u;i++)p->payload[i]=payload[i];
    return PORT_DQ_IOCW_OK;
}
uint32_t port_dq_ioc_write_enqueue(struct port_dq_ioc_write_queue *q,
                                    uint32_t tag){
    if(!q||!tag)return PORT_DQ_IOCW_BAD;
    for(unsigned i=0;i<PORT_DQ_IOCW_DEPTH;i++){
        struct port_dq_ioc_write_packet *p=&q->packets[i];
        if(p->state&&p->tag==tag){
            if(p->state!=PORT_DQ_IOCW_PREPARED)return PORT_DQ_IOCW_FULL;
            if(q->submitted==UINT32_MAX)return PORT_DQ_IOCW_BAD;
            p->state=PORT_DQ_IOCW_QUEUED;q->submitted++;
            return PORT_DQ_IOCW_OK;
        }
    }
    return PORT_DQ_IOCW_BAD;
}
uint32_t port_dq_ioc_write_service(struct port_dq_ioc_write_queue *q,
    struct port_dq_ioc_write_iosb *iosb){
    if(!q||!q->backend||!iosb||q->completed+q->failed>q->submitted)
        return PORT_DQ_IOCW_BAD;
    unsigned slot=PORT_DQ_IOCW_DEPTH;uint32_t order=UINT32_MAX;
    for(unsigned i=0;i<PORT_DQ_IOCW_DEPTH;i++){
        const struct port_dq_ioc_write_packet *p=&q->packets[i];
        if(p->state==PORT_DQ_IOCW_QUEUED&&p->order<order){slot=i;order=p->order;}
        else if(p->state!=0u&&p->state!=PORT_DQ_IOCW_PREPARED&&
                p->state!=PORT_DQ_IOCW_QUEUED)
            return PORT_DQ_IOCW_BAD;
    }
    if(slot==PORT_DQ_IOCW_DEPTH)return PORT_DQ_IOCW_BAD;
    struct port_dq_ioc_write_packet *p=&q->packets[slot];
    p->state=PORT_DQ_IOCW_ACTIVE;
    const uint32_t hw=q->backend(q->context,p->lbn,p->payload);
    const uint16_t status=hw==PORT_DQ_IOCW_OK?PORT_DQ_IOCW_OK:PORT_DQ_IOCW_IO;
    const uint16_t bytes=status==PORT_DQ_IOCW_OK?512u:0u;
    if(status==PORT_DQ_IOCW_OK){if(q->completed==UINT32_MAX)return PORT_DQ_IOCW_BAD;q->completed++;}
    else {if(q->failed==UINT32_MAX)return PORT_DQ_IOCW_BAD;q->failed++;}
    const uint32_t tag=p->tag;
    *p=(struct port_dq_ioc_write_packet){0};
    *iosb=(struct port_dq_ioc_write_iosb){tag,status,bytes};
    return status==PORT_DQ_IOCW_OK?PORT_DQ_IOCW_OK:PORT_DQ_IOCW_IO;
}
