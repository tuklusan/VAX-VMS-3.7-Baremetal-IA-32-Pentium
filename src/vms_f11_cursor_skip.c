#include "vms_f11_cursor.h"
/* M155: reject offset wrap without modifying the current VBN cursor. */
uint32_t port_f11_cursor_skip(struct port_f11_cursor *c,uint32_t bytes){
    if(!c||!c->volume||!c->chain)return PORT_F11_CURSOR_BAD;
    if(c->position>c->total_bytes||bytes>c->total_bytes-c->position)
        return PORT_F11_CURSOR_EOF;
    c->position+=bytes;return PORT_F11_CURSOR_OK;
}
