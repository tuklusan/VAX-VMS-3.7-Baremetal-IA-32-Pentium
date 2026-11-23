#include "vms_dq_ioc.h"
/* RETRYERR: decrement error count, return to dispatch only if retries remain.
 * A real RB730 would issue F_RECAL; IA-32 ATA reset/IRQ remains unported. */
uint32_t port_dq_ioc_recover(struct port_dq_ioc_queue *q){
 if(!q)return PORT_DQ_IOC_BAD;
 if(q->active>=PORT_DQ_IOC_DEPTH)return PORT_DQ_IOC_NOTFOUND;
 struct port_dq_ioc_packet *p=&q->packets[q->active];
 if(p->state!=PORT_DQ_IOC_FAILED)return PORT_DQ_IOC_BUSY;
 if(!p->retries_remaining)return PORT_DQ_IOC_IO;
 if(q->retried==UINT32_MAX)return PORT_DQ_IOC_CORRUPT;
 p->retries_remaining--;q->retried++;p->state=PORT_DQ_IOC_ACTIVE;
 return PORT_DQ_IOC_OK;
}
