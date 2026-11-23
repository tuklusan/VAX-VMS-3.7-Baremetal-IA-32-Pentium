#include "vms_f11_cursor.h"
/* M158: FIL$READVBN byte-order-safe two-byte field; no half value returned. */
uint32_t port_f11_cursor_word(struct port_f11_cursor *c,uint16_t *value){
    if(!value)return PORT_F11_CURSOR_BAD;
    uint8_t b[2];uint32_t done=0;
    const uint32_t rc=port_f11_cursor_read(c,b,2u,sizeof b,&done);
    if(rc!=PORT_F11_CURSOR_OK)return rc;
    *value=(uint16_t)((uint16_t)b[0]|((uint16_t)b[1]<<8));
    return PORT_F11_CURSOR_OK;
}
