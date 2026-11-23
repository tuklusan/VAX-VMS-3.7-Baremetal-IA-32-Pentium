#include "vms_f11_cursor.h"
/* M153: preflight file allocation and supplied read callback once. */
uint32_t port_f11_cursor_init(struct port_f11_cursor *c,
    const struct port_f11_boot_volume *v,const struct port_f11_chain *chain,
    uint32_t initial){
    if(!c||!v||!v->read_sector||!chain||!chain->count||
       chain->count>PORT_F11_CHAIN_MAX_EXTENTS)return PORT_F11_CURSOR_BAD;
    uint64_t total=0;
    for(uint32_t i=0;i<chain->count;i++){
        const struct port_f11_extent *e=&chain->extents[i];
        if(!e->blocks||!e->lbn||(uint64_t)e->lbn+e->blocks>0x10000000u)
            return PORT_F11_CURSOR_MAP;
        total+=(uint64_t)e->blocks*512u;
        if(total>UINT32_MAX)return PORT_F11_CURSOR_MAP;
    }
    if(initial>total)return PORT_F11_CURSOR_EOF;
    const struct port_f11_cursor tmp={v,chain,initial,(uint32_t)total,0u};
    *c=tmp;return PORT_F11_CURSOR_OK;
}
