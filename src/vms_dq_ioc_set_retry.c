#include "vms_dq_ioc.h"
/* RETRYERR initializes the UCB error retry budget before dispatch.
 * Only a prepared IRP can have its retry policy changed. */
uint32_t port_dq_ioc_set_retry(struct port_dq_ioc_queue *q,uint32_t tag,uint8_t limit){
 if(!q||!tag||limit>PORT_DQ_IOC_MAX_RETRIES)return PORT_DQ_IOC_BAD;
 for(unsigned i=0;i<PORT_DQ_IOC_DEPTH;i++){
  struct port_dq_ioc_packet *p=&q->packets[i];
  if(p->state==PORT_DQ_IOC_EMPTY||p->tag!=tag)continue;
  if(p->state!=PORT_DQ_IOC_PREPARED)return PORT_DQ_IOC_BUSY;
  p->retry_limit=limit;p->retries_remaining=limit;return PORT_DQ_IOC_OK;
 }
 return PORT_DQ_IOC_NOTFOUND;
}
