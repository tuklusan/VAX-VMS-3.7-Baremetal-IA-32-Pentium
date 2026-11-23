#include "vms_dqddt.h"
/* Only private privileged physical-block actions have native bridges;
 * buffered and ACP actions still require the original VMS dispatch chain. */
uint32_t port_dqddt_function(const struct port_dqddt_stage *s,
    const struct port_dq_fdt *fdt,uint16_t f){
    if(port_dqddt_validate(s,fdt)!=PORT_DQDDT_OK)return PORT_DQDDT_BAD;
    const uint32_t k=port_dq_fdt_dispatch(fdt,sizeof(*fdt),f);
    if(k==PORT_DQ_FDT_ILLEGAL)return PORT_DQDDT_BAD;
    if(f==PORT_DQ_READPBLK||f==PORT_DQ_WRITEPBLK){
        const unsigned row=f==PORT_DQ_READPBLK?PORT_DQ_FDT_READ:
                                                   PORT_DQ_FDT_WRITE;
        const uint32_t off=port_dqddt_entry_offset(row);
        const uint32_t action_pc=f==PORT_DQ_READPBLK?
            s->native_read_action_pc:s->native_write_action_pc;
        if(off==UINT32_MAX||
           port_v37_read32(s->image+off+8u)!=
             (action_pc|UINT32_C(0x80000000)))
            return PORT_DQDDT_BAD;
        return PORT_DQDDT_NATIVE;
    }
    return PORT_DQDDT_UNBOUND;
}
