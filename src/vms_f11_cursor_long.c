#include "vms_f11_cursor.h"
/* M159: little-endian Files-11 FH2 longword across VBN boundaries. */
uint32_t port_f11_cursor_long(struct port_f11_cursor *c,uint32_t *value){
    if(!value)return PORT_F11_CURSOR_BAD;
    uint8_t b[4];uint32_t done=0;
    const uint32_t rc=port_f11_cursor_read(c,b,4u,sizeof b,&done);
    if(rc!=PORT_F11_CURSOR_OK)return rc;
    *value=(uint32_t)b[0]|((uint32_t)b[1]<<8)|
           ((uint32_t)b[2]<<16)|((uint32_t)b[3]<<24);
    return PORT_F11_CURSOR_OK;
}
