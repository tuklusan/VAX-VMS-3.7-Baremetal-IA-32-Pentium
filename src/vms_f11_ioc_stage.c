#include "vms_f11_ioc.h"
/* M181: private EXE$INSIOQ staging, never publishes DQA0 DDT. */
uint32_t port_f11_ioc_stage(struct port_f11_ioc_file *f){
 if(!f||!f->queue||!f->chain)return PORT_F11_IOC_BAD;
 if(f->pending)return PORT_F11_IOC_BUSY;
 if(f->next_tag==UINT32_MAX)return PORT_F11_IOC_BAD;
 /* This bridge owns the IOC queue for its whole sector transaction.
  * Do not steal completions from other requestors. */
 if(f->queue->active!=PORT_DQ_IOC_DEPTH)return PORT_F11_IOC_BUSY;
 for(unsigned i=0;i<PORT_DQ_IOC_DEPTH;i++)
  if(f->queue->packets[i].state!=PORT_DQ_IOC_EMPTY)
   return PORT_F11_IOC_BUSY;
 uint32_t lbn=0,within=0;
 uint32_t st=port_f11_ioc_locate(f,&lbn,&within);
 if(st!=PORT_F11_IOC_OK)return st;
 st=port_dq_ioc_prepare(f->queue,f->next_tag,lbn,f->sector,512u);
 if(st!=PORT_DQ_IOC_OK)return PORT_F11_IOC_BUSY;
 st=port_dq_ioc_enqueue(f->queue,f->next_tag);
 if(st!=PORT_DQ_IOC_OK){
  (void)port_dq_ioc_cancel(f->queue,f->next_tag);
  return PORT_F11_IOC_BUSY;
 }
 f->pending=1;f->pending_tag=f->next_tag;
 f->pending_within=within;f->next_tag++;
 return PORT_F11_IOC_OK;
}
