#include "vms_ddtreloc.h"
/* Source lines 336-347: skip two bitmap words and step 12-byte rows;
 * each action is at row+8, relative to the current dispatch block. */
uint32_t port_ddtreloc_functions(const struct port_ddtreloc_image *im,
    uint32_t off,uint16_t len){
    if(!im||!im->image||off>im->length||len>im->length-off||
       len<16u||(len-16u)%12u)return PORT_DDTRELOC_BAD;
    for(uint32_t at=off+24u;at+4u<=off+(uint32_t)len;at+=12u){
        uint8_t *p=im->image+at;
        const uint32_t old=port_ddtreloc_read32(p);
        if(!(old&UINT32_C(0x80000000)))
            port_ddtreloc_write32(p,old+im->pc_base+at-8u);
    }
    return PORT_DDTRELOC_OK;
}
