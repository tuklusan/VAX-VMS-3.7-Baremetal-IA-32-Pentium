#include "vms_ddtreloc.h"
/* Source DDT has a 38-byte dispatch header and an FDT of 16+12*n.
 * Validate the full source span before touching the supplied image. */
uint32_t port_ddtreloc_preflight(const struct port_ddtreloc_image *im,
    uint32_t *fdt_offset,uint16_t *fdt_length){
    if(!im||!im->image||!fdt_offset||!fdt_length||
       im->length<PORT_DDTRELOC_DDT_LEN||!im->pc_base||
       (uint64_t)im->pc_base+im->length>UINT32_MAX)return PORT_DDTRELOC_BAD;
    const uint16_t len=port_ddtreloc_read16(im->image+PORT_DDTRELOC_FDT_SIZE_OFF);
    const uint32_t raw=port_ddtreloc_read32(im->image+PORT_DDTRELOC_FDT_OFF);
    /* Before relocation this word is relative; after relocation high bit
     * indicates it already points to native system virtual address. */
    const uint32_t off=(raw&UINT32_C(0x80000000))?
       (raw>=im->pc_base?raw-im->pc_base:UINT32_MAX):raw;
    if(off<PORT_DDTRELOC_DDT_LEN||off>im->length||
       len<16u||(len-16u)%12u||len>im->length-off)
        return PORT_DDTRELOC_UNBOUND;
    *fdt_offset=off;*fdt_length=len;return PORT_DDTRELOC_OK;
}
