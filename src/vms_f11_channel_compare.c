#include "vms_f11_channel_priv.h"
/* M191: compare media bytes to resident image, not a synthetic prompt. */
uint32_t port_f11_chan_compare(struct port_f11_chan_table *t,
 uint32_t owner,uint32_t handle,const uint8_t *expected,
 uint32_t count,uint32_t *equal){
 struct port_f11_chan_slot *s=port_f11_chan_get(t,owner,handle);
 if(!s)return PORT_F11_CHAN_NOCHANNEL;
 if(!expected||!equal)return PORT_F11_CHAN_BAD;
 return port_f11_chan_map_status(port_f11_ioc_compare(
          &s->file,expected,count,equal));
}
