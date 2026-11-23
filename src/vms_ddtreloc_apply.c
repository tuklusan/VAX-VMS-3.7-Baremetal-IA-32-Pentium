#include "vms_ddtreloc.h"
/* Preserve the original relocation order: DDT followed by FDT. Reject
 * malformed private images before committing any DDT field changes. */
uint32_t port_ddtreloc_apply(const struct port_ddtreloc_image *im){
    uint32_t fdt=0;uint16_t size=0;
    uint32_t result=port_ddtreloc_preflight(im,&fdt,&size);
    if(result!=PORT_DDTRELOC_OK)return result;
    if(im->pc_base>UINT32_MAX-(uint32_t)im->length)return PORT_DDTRELOC_BAD;
    /* The original does not roll back arbitrary writes. Our bounded private
     * image has already passed every offset/size check. */
    result=port_ddtreloc_header(im);
    if(result!=PORT_DDTRELOC_OK)return result;
    return port_ddtreloc_functions(im,fdt,size);
}
