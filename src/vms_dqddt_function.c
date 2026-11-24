#include "vms_dqddt.h"
/* Only private privileged physical-block actions have native bridges;
 * buffered and ACP actions still require the original VMS dispatch chain.
 * FUNCTAB selects by the low six-bit function code, retaining modifiers. */
uint32_t port_dqddt_function(const struct port_dqddt_stage *s,
    const struct port_dq_fdt *fdt,uint16_t f){
    if(port_dqddt_validate(s,fdt)!=PORT_DQDDT_OK)return PORT_DQDDT_BAD;
    const uint32_t k=port_dq_fdt_dispatch(fdt,sizeof(*fdt),f);
    if(k==PORT_DQ_FDT_ILLEGAL)return PORT_DQDDT_BAD;
    /* FUNCTAB indexes the six-bit IO function number; modifier bits remain
     * in IRP$W_FUNC for the action routine and do not select another row. */
    const uint16_t code=(uint16_t)(f&63u);
    if(code==PORT_DQ_READPBLK||code==PORT_DQ_WRITEPBLK){
        const unsigned row=code==PORT_DQ_READPBLK?PORT_DQ_FDT_READ:
                                                   PORT_DQ_FDT_WRITE;
        const uint32_t off=port_dqddt_entry_offset(row);
        const uint32_t action_pc=code==PORT_DQ_READPBLK?
            s->native_read_action_pc:s->native_write_action_pc;
        if(off==UINT32_MAX||
           port_v37_read32(s->image+off+8u)!=
             (action_pc|UINT32_C(0x80000000)))
            return PORT_DQDDT_BAD;
        return PORT_DQDDT_NATIVE;
    }
    return PORT_DQDDT_UNBOUND;
}
