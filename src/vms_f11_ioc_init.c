#include "vms_f11_ioc.h"
/* M177: validate source FH2 extent chain before installing private file. */
uint32_t port_f11_ioc_init(struct port_f11_ioc_file *f,
 const struct port_f11_chain *c,struct port_dq_ioc_queue *q,uint32_t length){
 if(!f||!c||!q||!q->read_sector||!c->count||
    c->count>PORT_F11_CHAIN_MAX_EXTENTS||!c->blocks||
    (uint64_t)length>(uint64_t)c->blocks*512u||
    port_dq_ioc_validate(q)!=PORT_DQ_IOC_OK) return PORT_F11_IOC_BAD;
 uint64_t blocks=0;
 for(uint32_t i=0;i<c->count;i++){
  const struct port_f11_extent *e=&c->extents[i];
  if(!e->blocks||!e->lbn||(uint64_t)e->lbn+e->blocks>0x10000000u)
   return PORT_F11_IOC_MAP;
  blocks+=e->blocks;
 }
 if(blocks!=c->blocks)return PORT_F11_IOC_MAP;
 struct port_f11_ioc_file t={0};
 t.chain=c;t.queue=q;t.length=length;t.next_tag=0xf1100000u;
 *f=t;return PORT_F11_IOC_OK;
}
