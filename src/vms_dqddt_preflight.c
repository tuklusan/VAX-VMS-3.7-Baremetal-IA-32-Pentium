#include "vms_dqddt.h"
/* No DDT publication or interrupt-owned CRB before source DQ STARTIO and
 * ACP handlers exist. Stage must live at a distinct aligned PC address. */
uint32_t port_dqddt_preflight(const struct port_dqddt_stage *s,
    const struct port_dq_fdt *fdt,uint32_t base,uint32_t start,uint32_t ret){
    if(!s||!base||(base&3u)||!start||!ret||
       base>UINT32_MAX-PORT_DQDDT_IMAGE_BYTES||
       !((uint64_t)start<(uint64_t)base ||
         (uint64_t)start>=(uint64_t)base+PORT_DQDDT_IMAGE_BYTES)||
       !((uint64_t)ret<(uint64_t)base ||
         (uint64_t)ret>=(uint64_t)base+PORT_DQDDT_IMAGE_BYTES)||
       port_dq_fdt_validate(fdt,sizeof(*fdt))!=PORT_DQ_FDT_OK)
        return PORT_DQDDT_BAD;
    return PORT_DQDDT_OK;
}
