#include "vms_dqddt.h"

uint32_t port_dqddt_dispatch_bootread(const struct port_dqddt_stage *stage,
    const struct port_dq_fdt *fdt,const struct port_dq_bootread_request *q,
    struct port_dq_bootread_result *result,uint32_t lbn){
    if(port_dqddt_function(stage,fdt,PORT_DQ_READPBLK)!=PORT_DQDDT_NATIVE)
        return PORT_DQ_BOOTREAD_UNBOUND;
#if UINTPTR_MAX > UINT32_MAX
    /* A 32-bit resident DDT function address cannot be called in a 64-bit
     * host process. Host tests exercise the table and underlying action. */
    (void)q; (void)result; (void)lbn;
    return PORT_DQ_BOOTREAD_UNBOUND;
#else
    typedef uint32_t (*read_action_fn)(const struct port_dqddt_stage *,
        const struct port_dq_fdt *,const struct port_dq_bootread_request *,
        struct port_dq_bootread_result *,uint32_t);
    read_action_fn action=(read_action_fn)(uintptr_t)(stage->native_read_action_pc&UINT32_C(0x7fffffff));
    return action(stage,fdt,q,result,lbn);
#endif
}

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
