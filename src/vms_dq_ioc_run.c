#include "vms_dq_ioc.h"
/* Drain bounded IOC queue in FIFO order; error posting does not suppress later
 * unrelated IRPs. The report is committed only after the complete drain. */
uint32_t port_dq_ioc_run(struct port_dq_ioc_queue *q,
                          struct port_dq_ioc_report *report){
 if(!q||!report)return PORT_DQ_IOC_BAD;
 if(q->active!=PORT_DQ_IOC_DEPTH)return PORT_DQ_IOC_BUSY;
 struct port_dq_ioc_report r={0};uint32_t before=q->reads;
 for(unsigned n=0;n<PORT_DQ_IOC_DEPTH;n++){
  struct port_dq_ioc_iosb iosb;
  uint32_t st=port_dq_ioc_service(q,&iosb);
  if(st==PORT_DQ_IOC_NOTFOUND)break;
  if(st!=PORT_DQ_IOC_OK&&st!=PORT_DQ_IOC_IO)return st;
  r.processed++;
  if(st==PORT_DQ_IOC_OK)r.succeeded++;else r.failed++;
 }
 for(unsigned n=0;n<PORT_DQ_IOC_DEPTH;n++)
  if(q->packets[n].state==PORT_DQ_IOC_QUEUED)
   return PORT_DQ_IOC_CORRUPT;
 r.reads=q->reads-before;*report=r;
 return r.failed?PORT_DQ_IOC_IO:PORT_DQ_IOC_OK;
}
