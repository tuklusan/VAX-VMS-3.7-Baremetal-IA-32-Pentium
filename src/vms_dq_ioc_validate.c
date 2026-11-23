#include "vms_dq_ioc.h"
/* M162: validate in-memory IOC ownership and single active IRP invariant. */
uint32_t port_dq_ioc_validate(const struct port_dq_ioc_queue *q){
    if(!q||!q->read_sector||q->active>PORT_DQ_IOC_DEPTH||
       q->completed+q->failed>q->submitted)return PORT_DQ_IOC_BAD;
    unsigned active=0u;
    for(unsigned i=0;i<PORT_DQ_IOC_DEPTH;i++){
        const struct port_dq_ioc_packet *p=&q->packets[i];
        if(p->state>PORT_DQ_IOC_FAILED)return PORT_DQ_IOC_CORRUPT;
        if(p->state==PORT_DQ_IOC_EMPTY)continue;
        if(!p->tag||!p->destination||p->lbn>=0x10000000u||
           !p->order)return PORT_DQ_IOC_CORRUPT;
        active+=p->state==PORT_DQ_IOC_ACTIVE;
        for(unsigned j=i+1;j<PORT_DQ_IOC_DEPTH;j++){
            const struct port_dq_ioc_packet *other=&q->packets[j];
            if(other->state!=PORT_DQ_IOC_EMPTY&&
               (other->tag==p->tag||other->order==p->order))
                return PORT_DQ_IOC_CORRUPT;
        }
    }
    if(active>1u|| (q->active==PORT_DQ_IOC_DEPTH)!=(active==0u)||
       (active&&q->packets[q->active].state!=PORT_DQ_IOC_ACTIVE))
        return PORT_DQ_IOC_CORRUPT;
    return PORT_DQ_IOC_OK;
}
