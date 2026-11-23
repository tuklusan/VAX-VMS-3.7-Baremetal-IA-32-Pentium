#include "vms_dq_ioc.h"
/* M168: remove an unissued IRP; reject cancellation after DQ_STARTIO. */
uint32_t port_dq_ioc_cancel(struct port_dq_ioc_queue *q,uint32_t tag){
    if(!q||!tag)return PORT_DQ_IOC_BAD;
    for(unsigned i=0;i<PORT_DQ_IOC_DEPTH;i++){
        struct port_dq_ioc_packet *p=&q->packets[i];
        if(p->state!=PORT_DQ_IOC_EMPTY&&p->tag==tag){
            if(p->state!=PORT_DQ_IOC_PREPARED&&
               p->state!=PORT_DQ_IOC_QUEUED)return PORT_DQ_IOC_BUSY;
            if(p->state==PORT_DQ_IOC_QUEUED){
                if(!q->submitted)return PORT_DQ_IOC_CORRUPT;
                q->submitted--; /* cancelled before device ownership */
            }
            *p=(struct port_dq_ioc_packet){0};return PORT_DQ_IOC_OK;
        }
    }
    return PORT_DQ_IOC_NOTFOUND;
}
