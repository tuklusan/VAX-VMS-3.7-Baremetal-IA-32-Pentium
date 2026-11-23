#include "vms_dqddt.h"
#include "vms_ini_devicedb.h"
/* Only a complete source-sized DDT staging proof, NOT a usable DQ driver. */
uint32_t port_dqddt_validate(const struct port_dqddt_stage *s,
    const struct port_dq_fdt *fdt){
    if(!s||!s->ready||s->driver_bound||s->reserved[0]||s->reserved[1]||
       s->native_read_action_pc!=(uint32_t)(uintptr_t)port_dqddt_bootread||
       s->native_write_action_pc!=(uint32_t)(uintptr_t)port_dqddt_bootwrite||
       port_dqddt_preflight(s,fdt,s->table_base,
          s->native_start_pc,s->ioc_return_pc)!=PORT_DQDDT_OK||
       s->dpt.max_units!=4u||s->dpt.default_units!=4u||
       s->dpt.buffer_bytes!=512u||s->dpt.fork_ipl!=8u||
       s->dpt.device_ipl!=21u||s->dpt.retry_max!=8u||
       s->dpt.svp_required!=1u)return PORT_DQDDT_BAD;
    const uint8_t *b=s->image;
    if(port_v37_read32(b)!=(s->native_start_pc-s->table_base)||
       port_v37_read32(b+4u)!=s->ioc_return_pc||
       port_v37_read32(b+8u)!=PORT_DQDDT_HEADER_BYTES||
       port_v37_read32(b+12u)!=s->ioc_return_pc||
       port_v37_read32(b+16u)!=0u||
       port_v37_read16(b+20u)!=PORT_DQDDT_SOURCE_DIAG_BYTES||
       port_v37_read16(b+22u)!=0u||
       port_v37_read32(b+24u)!=s->ioc_return_pc||
       port_v37_read32(b+28u)!=s->ioc_return_pc||
       port_v37_read32(b+32u)!=0u||
       port_v37_read16(b+36u)!=PORT_DQDDT_FDT_BYTES||
       b[38u]!=0u||b[39u]!=0u||
       port_dqddt_fdt_check(s,fdt)!=PORT_DQDDT_OK)
        return PORT_DQDDT_BAD;
    return PORT_DQDDT_OK;
}
