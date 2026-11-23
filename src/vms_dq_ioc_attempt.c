#include "vms_dq_ioc.h"
/* One actual hardware read attempt. The destination is untouched until IOC post.
 * Overflow of the per-packet attempt counter is rejected before hardware I/O. */
uint32_t port_dq_ioc_attempt(struct port_dq_ioc_queue *q){
 if(!q||!q->read_sector)return PORT_DQ_IOC_BAD;
 if(q->active>=PORT_DQ_IOC_DEPTH)return PORT_DQ_IOC_NOTFOUND;
 struct port_dq_ioc_packet *p=&q->packets[q->active];
 if(p->state!=PORT_DQ_IOC_ACTIVE)return PORT_DQ_IOC_BUSY;
 if(p->attempts==UINT8_MAX)return PORT_DQ_IOC_CORRUPT;
 p->attempts++;
 return port_dq_ioc_transfer(q);
}
