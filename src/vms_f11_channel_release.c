#include "vms_f11_channel_priv.h"
/* M188: old handles cannot close a reused slot; pending I/O is retained. */
uint32_t port_f11_chan_release(struct port_f11_chan_table *t,
 uint32_t owner,uint32_t handle){
 struct port_f11_chan_slot *s=port_f11_chan_get(t,owner,handle);
 if(!s)return PORT_F11_CHAN_NOCHANNEL;
 if(s->file.pending)return PORT_F11_CHAN_BUSY;
 struct port_f11_chan_slot blank={0};
 *s=blank;t->open_count--;t->releases++;
 return PORT_F11_CHAN_OK;
}
