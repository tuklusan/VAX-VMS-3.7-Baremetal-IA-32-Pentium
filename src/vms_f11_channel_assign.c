#include "vms_f11_channel_priv.h"
/* M187: generation-protected source-style private channel assignment. */
uint32_t port_f11_chan_assign(struct port_f11_chan_table *t,
 uint32_t owner,uint32_t *handle){
 if(!t||!owner||!handle||
    port_f11_chan_validate(t)!=PORT_F11_CHAN_OK)return PORT_F11_CHAN_BAD;
 if(t->next_generation>PORT_F11_CHAN_MAX_GENERATION)return PORT_F11_CHAN_BUSY;
 for(unsigned i=0;i<PORT_F11_CHAN_SLOTS;i++){
  struct port_f11_chan_slot *s=&t->slots[i];
  if(s->assigned)continue;
  struct port_f11_ioc_file file={0};
  if(port_f11_ioc_init(&file,t->chain,t->queue,t->length)!=PORT_F11_IOC_OK)
   return PORT_F11_CHAN_BAD;
  s->file=file;s->owner=owner;s->generation=t->next_generation++;
  s->assigned=1u;t->open_count++;t->assignments++;
  *handle=(s->generation<<8)|(i+1u);
  return PORT_F11_CHAN_OK;
 }
 return PORT_F11_CHAN_BUSY;
}
