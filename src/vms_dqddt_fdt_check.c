#include "vms_dqddt.h"
#include "vms_ini_devicedb.h"
/* Verify every mask and GENRADDR slot, including the single native READ
 * action and the still-unbound write/ACP actions. */
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
        const uint32_t action=i==PORT_DQ_FDT_READ?
            (s->native_read_action_pc|UINT32_C(0x80000000)):0u;
        if(i>=2u&&port_v37_read32(s->image+off+8u)!=action)
            return PORT_DQDDT_BAD;
    }
    if(port_dqddt_entry_offset(PORT_DQ_FDT_ROWS-1u)+12u!=
       PORT_DQDDT_IMAGE_BYTES||
       port_dqddt_entry_offset(PORT_DQ_FDT_ROWS)!=UINT32_MAX)
        return PORT_DQDDT_BAD;
    return PORT_DQDDT_OK;
}
