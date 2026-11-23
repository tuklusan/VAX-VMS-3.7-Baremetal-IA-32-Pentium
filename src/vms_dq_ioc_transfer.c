#include "vms_dq_ioc.h"
/* M166: physical DQ read dispatch via real registered ATA callback.
 * Scratch sector is never published on failed hardware transfer. */
uint32_t port_dq_ioc_transfer(struct port_dq_ioc_queue *q){
    if(!q||!q->read_sector)return PORT_DQ_IOC_BAD;
    if(q->active>=PORT_DQ_IOC_DEPTH)return PORT_DQ_IOC_NOTFOUND;
    struct port_dq_ioc_packet *p=&q->packets[q->active];
    if(p->state!=PORT_DQ_IOC_ACTIVE)return PORT_DQ_IOC_CORRUPT;
    uint8_t scratch[512];
    const uint32_t st=q->read_sector(q->context,p->lbn,scratch);
    q->reads++;
    if(st!=1u){p->state=PORT_DQ_IOC_FAILED;return PORT_DQ_IOC_IO;}
    for(unsigned i=0;i<512u;i++)p->sector[i]=scratch[i];
    p->state=PORT_DQ_IOC_FINISHED;
    return PORT_DQ_IOC_OK;
}
