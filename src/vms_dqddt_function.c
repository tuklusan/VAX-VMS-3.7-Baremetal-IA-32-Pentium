#include "vms_dqddt.h"
/* A source legal function mask is not an executable DDT/ACP action.
 * Reject all $QIO uses until corresponding native register ABI exists. */
uint32_t port_dqddt_function(const struct port_dqddt_stage *s,
    const struct port_dq_fdt *fdt,uint16_t f){
    if(port_dqddt_validate(s,fdt)!=PORT_DQDDT_OK)return PORT_DQDDT_BAD;
    const uint32_t k=port_dq_fdt_dispatch(fdt,sizeof(*fdt),f);
    if(k==PORT_DQ_FDT_ILLEGAL)return PORT_DQDDT_BAD;
    return PORT_DQDDT_UNBOUND;
}
