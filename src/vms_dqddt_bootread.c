#include "vms_dqddt.h"

uint32_t port_dqddt_dispatch_bootread(const struct port_dqddt_stage *stage,
    const struct port_dq_fdt *fdt,const struct port_dq_bootread_request *q,
    struct port_dq_bootread_result *result,uint32_t lbn){
    if(port_dqddt_function(stage,fdt,PORT_DQ_READPBLK)!=PORT_DQDDT_NATIVE)
        return PORT_DQ_BOOTREAD_UNBOUND;
#if UINTPTR_MAX > UINT32_MAX
    /* Host pointers do not fit the resident 32-bit table representation.
     * Validation above binds this slot to this exact implementation, so
     * exercise the same action through its typed host symbol. */
    return port_dqddt_bootread(stage,fdt,q,result,lbn);
#else
    typedef uint32_t (*read_action_fn)(const struct port_dqddt_stage *,
        const struct port_dq_fdt *,const struct port_dq_bootread_request *,
        struct port_dq_bootread_result *,uint32_t);
    read_action_fn action=(read_action_fn)(uintptr_t)(stage->native_read_action_pc&UINT32_C(0x7fffffff));
    return action(stage,fdt,q,result,lbn);
#endif
}

uint32_t port_dqddt_dispatch_bootwrite(const struct port_dqddt_stage *stage,
    const struct port_dq_fdt *fdt,const struct port_dq_bootread_request *q,
    struct port_dq_bootread_result *result,uint32_t lbn){
    if(port_dqddt_function(stage,fdt,PORT_DQ_WRITEPBLK)!=PORT_DQDDT_NATIVE)
        return PORT_DQ_BOOTREAD_UNBOUND;
#if UINTPTR_MAX > UINT32_MAX
    return port_dqddt_bootwrite(stage,fdt,q,result,lbn);
#else
    typedef uint32_t (*write_action_fn)(const struct port_dqddt_stage *,
        const struct port_dq_fdt *,const struct port_dq_bootread_request *,
        struct port_dq_bootread_result *,uint32_t);
    write_action_fn action=(write_action_fn)(uintptr_t)
        (stage->native_write_action_pc&UINT32_C(0x7fffffff));
    return action(stage,fdt,q,result,lbn);
#endif
}

/* Native DQ_STARTIO subset. The activated DDT START pointer names this
 * dispatcher; SYS.EXE supplies its private typed request contract. Only the
 * two privileged physical block actions have handlers.
 * Buffered I/O, ACP, process windows and RB730 controller commands remain
 * unbound and cannot fall through to a guessed native implementation. */
uint32_t port_dqddt_startio(const struct port_dqddt_stage *stage,
    const struct port_dqddt_native *native,const struct port_dq_fdt *fdt,
    const struct port_dq_bootread_request *request,
    struct port_dq_bootread_result *result){
    if(!request||!request->irp||request->irp_bytes!=PORT_V37_IRP_SIZE||!result||
       port_dqddt_native_validate(stage,fdt,native)!=PORT_DQDDT_OK)
        return PORT_DQ_BOOTREAD_BAD;
    struct port_dq_bootread_request active=*request;
    active.active_ddt_base=active.device_info&&
        active.device_info->incomplete_driver_mask==6u?native->base:0u;
    const struct port_dq_bootread_request *q=&active;
    const uint16_t function=port_v37_read16(q->irp+PORT_V37_IRP_FUNC);
    const uint32_t action=port_dqddt_function(stage,fdt,function);
    if(action==PORT_DQDDT_BAD)return PORT_DQ_BOOTREAD_BAD;
    if(action!=PORT_DQDDT_NATIVE)return PORT_DQ_BOOTREAD_UNBOUND;
    uint32_t st=port_dq_bootread_preflight(q);
    if(st!=PORT_DQ_BOOTREAD_OK)return st;
    const uint32_t lbn=port_v37_read32(q->irp+PORT_V37_IRP_MEDIA);
    const unsigned row=function==PORT_DQ_READPBLK?PORT_DQ_FDT_READ:
                                                    PORT_DQ_FDT_WRITE;
    const uint32_t off=port_dqddt_entry_offset(row);
    if(off==UINT32_MAX)return PORT_DQ_BOOTREAD_BAD;
#if UINTPTR_MAX > UINT32_MAX
    /* Host addresses cannot be represented by source DDT longwords. The
     * native table has still been validated; call its fixed typed target. */
    if(function==PORT_DQ_READPBLK)
        return port_dqddt_bootread(stage,fdt,q,result,lbn);
    return port_dqddt_bootwrite(stage,fdt,q,result,lbn);
#else
    typedef uint32_t (*action_fn)(const struct port_dqddt_stage *,
        const struct port_dq_fdt *,const struct port_dq_bootread_request *,
        struct port_dq_bootread_result *,uint32_t);
    action_fn action_pc=(action_fn)(uintptr_t)
        port_v37_read32(native->image+off+8u);
    return action_pc(stage,fdt,q,result,lbn);
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

uint32_t port_dqddt_bootwrite(const struct port_dqddt_stage *stage,
    const struct port_dq_fdt *fdt,const struct port_dq_bootread_request *q,
    struct port_dq_bootread_result *result,uint32_t lbn){
    if(port_dqddt_validate(stage,fdt)!=PORT_DQDDT_OK||!q||!result)
        return PORT_DQ_BOOTREAD_BAD;
    struct port_dq_fdt_decision decision={0};
    if(port_dq_fdt_classify(fdt,sizeof(*fdt),PORT_DQ_WRITEPBLK,&decision)!=
       PORT_DQ_FDT_OK||decision.action_class!=PORT_DQ_FDT_WRITE||
       decision.buffered||decision.driver_bound)
        return PORT_DQ_BOOTREAD_UNBOUND;
    return port_dq_bootwrite_execute_physical(q,result,lbn);
}
