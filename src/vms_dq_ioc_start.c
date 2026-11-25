#include "vms_dq_ioc.h"
/* M165: dispatch oldest queued IRP to original DQ_STARTIO state. */
uint32_t port_dq_ioc_start(struct port_dq_ioc_queue *q,uint32_t *tag){
    if(!q||!tag||!q->read_sector)return PORT_DQ_IOC_BAD;
    if(q->active!=PORT_DQ_IOC_DEPTH)return PORT_DQ_IOC_BUSY;
    unsigned chosen=PORT_DQ_IOC_DEPTH;uint32_t order=0u;uint8_t priority=0u;
    for(unsigned i=0;i<PORT_DQ_IOC_DEPTH;i++){
        const struct port_dq_ioc_packet *p=&q->packets[i];
        /* EXE$INSERTIRP walks past entries with strictly greater priority;
         * equal-priority requests are inserted ahead of older requests. */
        if(p->state==PORT_DQ_IOC_QUEUED&&
           (chosen==PORT_DQ_IOC_DEPTH||p->priority>priority||
            (p->priority==priority&&p->order>order))){
            chosen=i;order=p->order;priority=p->priority;
        }
    }
    if(chosen==PORT_DQ_IOC_DEPTH)return PORT_DQ_IOC_NOTFOUND;
    q->packets[chosen].state=PORT_DQ_IOC_ACTIVE;
    q->active=chosen;*tag=q->packets[chosen].tag;
    return PORT_DQ_IOC_OK;
}
