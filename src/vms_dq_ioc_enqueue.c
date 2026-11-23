#include "vms_dq_ioc.h"
/* M164: source EXE$INSIOQ/IOC post queue order, no AST or user IRP. */
uint32_t port_dq_ioc_enqueue(struct port_dq_ioc_queue *q,uint32_t tag){
    if(!q||!tag)return PORT_DQ_IOC_BAD;
    for(unsigned i=0;i<PORT_DQ_IOC_DEPTH;i++){
        struct port_dq_ioc_packet *p=&q->packets[i];
        if(p->state!=PORT_DQ_IOC_EMPTY&&p->tag==tag){
            if(p->state!=PORT_DQ_IOC_PREPARED)return PORT_DQ_IOC_BUSY;
            p->state=PORT_DQ_IOC_QUEUED;q->submitted++;
            return PORT_DQ_IOC_OK;
        }
    }
    return PORT_DQ_IOC_NOTFOUND;
}
