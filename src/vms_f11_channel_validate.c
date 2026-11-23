#include "vms_f11_channel_priv.h"
/* M186: validate ownership/counts/generation without touching media. */
uint32_t port_f11_chan_validate(const struct port_f11_chan_table *t){
 if(!t||!t->queue||!t->chain||!t->next_generation||
     t->next_generation>PORT_F11_CHAN_MAX_GENERATION+1u||
     t->open_count>PORT_F11_CHAN_SLOTS||
     port_dq_ioc_validate(t->queue)!=PORT_DQ_IOC_OK)
  return PORT_F11_CHAN_BAD;
 uint32_t live=0;
 for(unsigned i=0;i<PORT_F11_CHAN_SLOTS;i++){
  const struct port_f11_chan_slot *s=&t->slots[i];
  if(!s->assigned){if(s->owner||s->generation)return PORT_F11_CHAN_BAD;continue;}
  if(!s->owner||!s->generation||s->generation>=t->next_generation||
     s->file.chain!=t->chain||s->file.queue!=t->queue||
     s->file.length!=t->length||
     port_f11_ioc_validate(&s->file)!=PORT_F11_IOC_OK)
   return PORT_F11_CHAN_BAD;
  live++;
 }
 if(live!=t->open_count||t->assignments<t->releases||
    t->assignments-t->releases!=live)return PORT_F11_CHAN_BAD;
 return PORT_F11_CHAN_OK;
}
