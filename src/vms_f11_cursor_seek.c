#include "vms_f11_cursor.h"
/* M154: source FIL$READVBN one-origin VBN seeks; EOF position is valid. */
uint32_t port_f11_cursor_seek(struct port_f11_cursor *c,uint32_t off){
    if(!c||!c->volume||!c->chain)return PORT_F11_CURSOR_BAD;
    if(off>c->total_bytes)return PORT_F11_CURSOR_EOF;
    c->position=off;return PORT_F11_CURSOR_OK;
}
