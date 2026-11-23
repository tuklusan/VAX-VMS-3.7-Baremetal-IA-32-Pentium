#include "vms_f11_cursor.h"
/* M160: compare actual disk bytes to already resident supervisor image.
 * A disagreement is a fatal pre-activation image validation error. */
uint32_t port_f11_cursor_compare(struct port_f11_cursor *c,
    const uint8_t *reference,uint32_t length,uint32_t *matched){
    if(!c||!reference||!matched)return PORT_F11_CURSOR_BAD;
    *matched=0u;
    while(*matched<length){
        uint8_t b[512];
        uint32_t n=length-*matched;
        if(n>sizeof b)n=sizeof b;
        uint32_t done=0;
        const uint32_t rc=port_f11_cursor_read(c,b,n,sizeof b,&done);
        if(rc!=PORT_F11_CURSOR_OK)return rc;
        for(uint32_t i=0;i<n;i++){
            if(b[i]!=reference[*matched+i]){
                *matched+=i;return PORT_F11_CURSOR_DIFFERENT;
            }
        }
        *matched+=n;
    }
    return PORT_F11_CURSOR_OK;
}
