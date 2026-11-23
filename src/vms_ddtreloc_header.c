#include "vms_ddtreloc.h"
/* Source RELOC_DDT_TABLE byte offsets 67-75; the sign bit is DEC's
 * 'already system address' marker. A high-bit word is left intact. */
uint32_t port_ddtreloc_header(const struct port_ddtreloc_image *im){
    static const uint8_t offsets[]={0u,4u,8u,12u,16u,24u,28u,32u};
    if(!im||!im->image||im->length<PORT_DDTRELOC_DDT_LEN)
        return PORT_DDTRELOC_BAD;
    for(unsigned i=0;i<sizeof offsets;i++){
        uint8_t *p=im->image+offsets[i];
        const uint32_t prior=port_ddtreloc_read32(p);
        if(!(prior&UINT32_C(0x80000000)))
            port_ddtreloc_write32(p,prior+im->pc_base);
    }
    return PORT_DDTRELOC_OK;
}
