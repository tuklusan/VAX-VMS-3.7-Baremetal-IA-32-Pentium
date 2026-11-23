#include "vms_dq_ioc.h"
/* M163: DQ PREPROCESS, physical media field and request ID preflight. */
uint32_t port_dq_ioc_prepare(struct port_dq_ioc_queue *q,uint32_t tag,
                            uint32_t lbn,uint8_t *dst,size_t capacity){
    if(!q||!q->read_sector||!tag||!dst||capacity<512u||
       lbn>=0x10000000u||q->order==UINT32_MAX)return PORT_DQ_IOC_BAD;
    unsigned free_at=PORT_DQ_IOC_DEPTH;
    for(unsigned i=0;i<PORT_DQ_IOC_DEPTH;i++){
        if(q->packets[i].state==PORT_DQ_IOC_EMPTY){
            if(free_at==PORT_DQ_IOC_DEPTH)free_at=i;
        }else if(q->packets[i].tag==tag)return PORT_DQ_IOC_BUSY;
    }
    if(free_at==PORT_DQ_IOC_DEPTH)return PORT_DQ_IOC_FULL;
    struct port_dq_ioc_packet *p=&q->packets[free_at];
    *p=(struct port_dq_ioc_packet){0};
    p->tag=tag;p->lbn=lbn;p->destination=dst;
    p->order=++q->order;p->state=PORT_DQ_IOC_PREPARED;
    return PORT_DQ_IOC_OK;
}
