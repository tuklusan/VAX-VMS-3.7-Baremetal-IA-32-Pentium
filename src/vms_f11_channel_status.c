#include "vms_f11_channel_priv.h"
/* M192: copy a source-style channel position/physical I/O snapshot. */
uint32_t port_f11_chan_status(struct port_f11_chan_table *t,
 uint32_t owner,uint32_t handle,struct port_f11_chan_status *out){
 struct port_f11_chan_slot *s=port_f11_chan_get(t,owner,handle);
 if(!s)return PORT_F11_CHAN_NOCHANNEL;
 if(!out)return PORT_F11_CHAN_BAD;
 if(port_f11_chan_validate(t)!=PORT_F11_CHAN_OK)return PORT_F11_CHAN_BAD;
 const struct port_f11_chan_status result={owner,handle,
  s->file.position,s->file.length,s->file.reads};
 *out=result;return PORT_F11_CHAN_OK;
}
