#include "vms_dq_fdt.h"
/* Source SYSMAR FUNCTAB masks only the low six I/O function bits.
 * Preserve modifier bits in full_function; never claim a PC driver or ACP
 * is bound simply because original DQDRIVER permits this function. */
uint32_t port_dq_fdt_classify(const struct port_dq_fdt *p,size_t bytes,
    uint16_t function,struct port_dq_fdt_decision *out){
    if(!out||port_dq_fdt_validate(p,bytes)!=PORT_DQ_FDT_OK)
        return PORT_DQ_FDT_BAD;
    const uint8_t code=(uint8_t)(function&63u);
    const uint64_t bit=PORT_DQ_BIT(code);
    if(!(p->mask[PORT_DQ_FDT_LEGAL]&bit))return PORT_DQ_FDT_ILLEGAL;
    uint8_t action=PORT_DQ_FDT_NO_ACTION;
    for(unsigned i=PORT_DQ_FDT_READ;i<PORT_DQ_FDT_ROWS;i++){
        if(p->mask[i]&bit){
            if(action!=PORT_DQ_FDT_NO_ACTION)return PORT_DQ_FDT_BAD;
            action=(uint8_t)i;
        }
    }
    if(action==PORT_DQ_FDT_NO_ACTION)return PORT_DQ_FDT_BAD;
    *out=(struct port_dq_fdt_decision){
        .full_function=function,.number=code,
        .buffered=(uint8_t)((p->mask[PORT_DQ_FDT_BUFFERED]&bit)!=0),
        .action_class=action,.driver_bound=0u};
    return PORT_DQ_FDT_OK;
}
uint32_t port_dq_fdt_dispatch(const struct port_dq_fdt *p,size_t bytes,
                             uint16_t function){
    struct port_dq_fdt_decision decision={0};
    uint32_t st=port_dq_fdt_classify(p,bytes,function,&decision);
    if(st!=PORT_DQ_FDT_OK)return st;
    /* Until real native DQ_STARTIO/ACP/CRB, no function may run. */
    return PORT_DQ_FDT_UNBOUND;
}
