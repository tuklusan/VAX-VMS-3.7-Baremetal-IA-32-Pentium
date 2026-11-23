#include "vms_f11_ioc.h"
/* M178: cursor and IOC packet remain owned by the same supervisor file. */
uint32_t port_f11_ioc_validate(const struct port_f11_ioc_file *f){
 if(!f||!f->chain||!f->queue||f->position>f->length||
    !f->next_tag||f->pending>1u||
    port_dq_ioc_validate(f->queue)!=PORT_DQ_IOC_OK)
  return PORT_F11_IOC_BAD;
 if(!f->pending)return f->pending_tag?PORT_F11_IOC_BAD:PORT_F11_IOC_OK;
 if(!f->pending_tag||f->pending_within>=512u)return PORT_F11_IOC_BAD;
 uint32_t found=0;
 for(unsigned i=0;i<PORT_DQ_IOC_DEPTH;i++)
  if(f->queue->packets[i].state!=PORT_DQ_IOC_EMPTY&&
     f->queue->packets[i].tag==f->pending_tag)found++;
 return found==1u?PORT_F11_IOC_OK:PORT_F11_IOC_BAD;
}
