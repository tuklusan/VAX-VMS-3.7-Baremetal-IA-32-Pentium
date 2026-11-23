#include "vms_f11_cursor.h"
/* M156: GETRTRVPTR extent walk is one-based, and may be fragmented. */
uint32_t port_f11_cursor_locate(const struct port_f11_cursor *c,
    uint32_t *lbn,uint32_t *within){
    if(!c||!c->volume||!c->chain||!lbn||!within)
        return PORT_F11_CURSOR_BAD;
    if(c->position>=c->total_bytes)return PORT_F11_CURSOR_EOF;
    uint32_t at=0;
    if(port_f11_chain_lbn(c->chain,c->position/512u+1u,&at)!=PORT_F11_CHAIN_OK)
        return PORT_F11_CURSOR_MAP;
    *lbn=at;*within=c->position%512u;
    return PORT_F11_CURSOR_OK;
}
