#include "vms_dqddt.h"

uint32_t port_dqddt_bootread(const struct port_dqddt_stage *stage,
    const struct port_dq_fdt *fdt,const struct port_dq_bootread_request *q,
    struct port_dq_bootread_result *result,uint32_t lbn){
    if(port_dqddt_validate(stage,fdt)!=PORT_DQDDT_OK || !q || !result)
        return PORT_DQ_BOOTREAD_BAD;
    struct port_dq_fdt_decision decision={0};
    if(port_dq_fdt_classify(fdt,sizeof(*fdt),PORT_DQ_READPBLK,&decision)!=
       PORT_DQ_FDT_OK || decision.action_class!=PORT_DQ_FDT_READ ||
       decision.buffered || decision.driver_bound)
        return PORT_DQ_BOOTREAD_UNBOUND;
    return port_dq_bootread_execute_physical(q,result,lbn);
}
