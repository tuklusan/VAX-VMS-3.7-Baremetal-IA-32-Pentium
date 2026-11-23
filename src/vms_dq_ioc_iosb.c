#include "vms_dq_ioc.h"
/* DQ FUNCXT: completion status and actual byte count in two IOSB words.
 * Status 1 = source SS$_NORMAL, failure 10 is local and never VMS-facing. */
uint32_t port_dq_ioc_iosb(const struct port_dq_ioc_queue *q,
                           struct port_dq_ioc_iosb *iosb){
 if(!q||!iosb)return PORT_DQ_IOC_BAD;
 if(q->active>=PORT_DQ_IOC_DEPTH)return PORT_DQ_IOC_NOTFOUND;
 const struct port_dq_ioc_packet *p=&q->packets[q->active];
 if(p->state!=PORT_DQ_IOC_FINISHED&&p->state!=PORT_DQ_IOC_FAILED)
  return PORT_DQ_IOC_BUSY;
 *iosb=(struct port_dq_ioc_iosb){p->tag,
  p->state==PORT_DQ_IOC_FINISHED?PORT_DQ_IOC_OK:PORT_DQ_IOC_IO,
  p->state==PORT_DQ_IOC_FINISHED?512u:0u};
 return PORT_DQ_IOC_OK;
}
