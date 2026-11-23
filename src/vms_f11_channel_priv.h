#include "vms_f11_channel.h"
/* Source-private channel handle: 24-bit monotonic generation and 1-based slot.
 * Old handles never alias new ones during the finite boot lifetime. */
static inline struct port_f11_chan_slot *port_f11_chan_get(
 struct port_f11_chan_table *t,uint32_t owner,uint32_t handle){
 if(!t||!owner||!(handle&255u)||((handle&255u)>PORT_F11_CHAN_SLOTS))return 0;
 struct port_f11_chan_slot *s=&t->slots[(handle&255u)-1u];
 if(!s->assigned||s->owner!=owner||s->generation!=(handle>>8))return 0;
 return s;
}
static inline uint32_t port_f11_chan_map_status(uint32_t status){
 if(status==PORT_F11_IOC_OK)return PORT_F11_CHAN_OK;
 if(status==PORT_F11_IOC_EOF)return PORT_F11_CHAN_EOF;
 if(status==PORT_F11_IOC_IO)return PORT_F11_CHAN_IO;
 if(status==PORT_F11_IOC_DIFFERENT)return PORT_F11_CHAN_DIFFERENT;
 return status==PORT_F11_IOC_BUSY?PORT_F11_CHAN_BUSY:PORT_F11_CHAN_BAD;
}
