#include "vms_f11_channel_priv.h"
/* M189: preserve output position on invalid VBN/ownership. */
uint32_t port_f11_chan_seek(struct port_f11_chan_table *t,
 uint32_t owner,uint32_t handle,uint32_t pos){
 struct port_f11_chan_slot *s=port_f11_chan_get(t,owner,handle);
 if(!s)return PORT_F11_CHAN_NOCHANNEL;
 return port_f11_chan_map_status(port_f11_ioc_seek(&s->file,pos));
}
