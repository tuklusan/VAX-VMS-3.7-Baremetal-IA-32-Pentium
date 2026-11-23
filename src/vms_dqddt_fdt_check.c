#include "vms_dqddt.h"
#include "vms_ini_devicedb.h"
/* All original masks and all ten absent GENRADDR handler slots must be
 * checked, not merely legal/read/write mask samples. */
uint32_t port_dqddt_fdt_check(const struct port_dqddt_stage *s,
    const struct port_dq_fdt *fdt){
    if(!s||port_dq_fdt_validate(fdt,sizeof(*fdt))!=PORT_DQ_FDT_OK)
        return PORT_DQDDT_BAD;
    for(unsigned i=0;i<PORT_DQ_FDT_ROWS;i++){
        const uint32_t off=port_dqddt_entry_offset(i);
        if(off<PORT_DQDDT_HEADER_BYTES||off>PORT_DQDDT_IMAGE_BYTES-8u||
           port_v37_read32(s->image+off)!=(uint32_t)fdt->mask[i]||
           port_v37_read32(s->image+off+4u)!=(uint32_t)(fdt->mask[i]>>32u))
            return PORT_DQDDT_BAD;
        if(i>=2u&&port_v37_read32(s->image+off+8u)!=0u)
            return PORT_DQDDT_BAD;
    }
    if(port_dqddt_entry_offset(PORT_DQ_FDT_ROWS-1u)+12u!=
       PORT_DQDDT_IMAGE_BYTES||
       port_dqddt_entry_offset(PORT_DQ_FDT_ROWS)!=UINT32_MAX)
        return PORT_DQDDT_BAD;
    return PORT_DQDDT_OK;
}
