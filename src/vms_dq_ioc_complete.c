#include "vms_dq_ioc.h"
/* M167: source DQ FUNCXT/REQCOM: deliver exactly one final status.
 * The caller's destination is mutated only after successful physical IO. */
uint32_t port_dq_ioc_complete(struct port_dq_ioc_queue *q,uint32_t *tag){
    if(!q||!tag||q->active>=PORT_DQ_IOC_DEPTH)
        return PORT_DQ_IOC_BAD;
    struct port_dq_ioc_packet *p=&q->packets[q->active];
    if(p->state!=PORT_DQ_IOC_FINISHED&&p->state!=PORT_DQ_IOC_FAILED)
        return PORT_DQ_IOC_BUSY;
    const uint32_t st=p->state==PORT_DQ_IOC_FINISHED?
        PORT_DQ_IOC_OK:PORT_DQ_IOC_IO;
    *tag=p->tag;
    if(st==PORT_DQ_IOC_OK){
        for(unsigned i=0;i<512u;i++)p->destination[i]=p->sector[i];
        q->completed++;
    }else q->failed++;
    *p=(struct port_dq_ioc_packet){0};q->active=PORT_DQ_IOC_DEPTH;
    return st;
}
