#include "vms_dq_ioc.h"
/* CHECKECC -> RETRYERR: bounded attempts on the currently active physical IRP.
 * No ECC synthesis or RB730 recalibrate; every retry invokes the real ATA callback. */
uint32_t port_dq_ioc_retry(struct port_dq_ioc_queue *q){
 if(!q)return PORT_DQ_IOC_BAD;
 if(q->active>=PORT_DQ_IOC_DEPTH)return PORT_DQ_IOC_NOTFOUND;
 struct port_dq_ioc_packet *p=&q->packets[q->active];
 if(p->state!=PORT_DQ_IOC_ACTIVE&&p->state!=PORT_DQ_IOC_FAILED)
  return PORT_DQ_IOC_BUSY;
 for(unsigned n=0;n<=PORT_DQ_IOC_MAX_RETRIES;n++){
  if(p->state==PORT_DQ_IOC_FAILED){
   uint32_t st=port_dq_ioc_recover(q);
   if(st!=PORT_DQ_IOC_OK)return st;
  }
  uint32_t st=port_dq_ioc_attempt(q);
  if(st==PORT_DQ_IOC_OK)return st;
  if(st!=PORT_DQ_IOC_IO)return st;
 }
 return PORT_DQ_IOC_CORRUPT;
}
