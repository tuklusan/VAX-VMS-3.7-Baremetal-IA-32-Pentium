#include "vms_dqddt.h"
/* The staged READ action has one native bridge: privileged physical
 * READPBLK. Other legal actions still require the original DQ/ACP ABI. */
uint32_t port_dqddt_function(const struct port_dqddt_stage *s,
    const struct port_dq_fdt *fdt,uint16_t f){
    if(port_dqddt_validate(s,fdt)!=PORT_DQDDT_OK)return PORT_DQDDT_BAD;
    const uint32_t k=port_dq_fdt_dispatch(fdt,sizeof(*fdt),f);
    if(k==PORT_DQ_FDT_ILLEGAL)return PORT_DQDDT_BAD;
    if(f==PORT_DQ_READPBLK){
        const uint32_t off=port_dqddt_entry_offset(PORT_DQ_FDT_READ);
        if(off==UINT32_MAX||
           port_v37_read32(s->image+off+8u)!=
             (s->native_read_action_pc|UINT32_C(0x80000000)))
            return PORT_DQDDT_BAD;
        return PORT_DQDDT_NATIVE;
    }
    return PORT_DQDDT_UNBOUND;
}
