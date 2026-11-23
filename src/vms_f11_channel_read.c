#include "vms_f11_channel_priv.h"
/* M190: source FIL$READVBN through the actual private IOC physical queue.
 * Existing file-read transaction prevents failed partial destination writes. */
uint32_t port_f11_chan_read(struct port_f11_chan_table *t,
 uint32_t owner,uint32_t handle,uint8_t *dst,uint32_t count,
 size_t capacity,uint32_t *done){
 struct port_f11_chan_slot *s=port_f11_chan_get(t,owner,handle);
 if(!s)return PORT_F11_CHAN_NOCHANNEL;
 if(!dst||!done)return PORT_F11_CHAN_BAD;
 return port_f11_chan_map_status(port_f11_ioc_read(
          &s->file,dst,count,capacity,done));
}
