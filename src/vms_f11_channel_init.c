#include "vms_f11_channel_priv.h"
/* M185: initialize bounded supervisor-only channel table. */
uint32_t port_f11_chan_init(struct port_f11_chan_table *t,
 const struct port_f11_chain *c,struct port_dq_ioc_queue *q,uint32_t length){
 if(!t||!c||!q)return PORT_F11_CHAN_BAD;
 struct port_f11_ioc_file probe={0};
 if(port_f11_ioc_init(&probe,c,q,length)!=PORT_F11_IOC_OK)
  return PORT_F11_CHAN_BAD;
 struct port_f11_chan_table v={0};
 v.chain=c;v.queue=q;v.length=length;v.next_generation=1u;
 *t=v;return PORT_F11_CHAN_OK;
}
