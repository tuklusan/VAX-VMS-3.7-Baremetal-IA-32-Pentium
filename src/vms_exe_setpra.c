#include "vms_exe_setpra.h"
#include <stddef.h>
uint32_t port_exe_setpra(struct port_exe_power_process *process,
                         uint32_t ast_address,uint32_t access_mode) {
    if(!process)return PORT_SS_BADPARAM;
    uint8_t mode=(uint8_t)(access_mode&3u);
    if(mode<(process->previous_mode&3u))mode=process->previous_mode&3u;
    if(!process->ast_address) {
        if(!process->ast_available)return PORT_SS_EXQUOTA;
        --process->ast_available;
    }
    process->ast_address=ast_address;
    process->ast_mode=mode;
    process->power_pending=1u;
    return PORT_SS_NORMAL;
}
unsigned port_exe_powerast(struct port_exe_power_process *const *processes,
                            unsigned process_slots,uint32_t *down_centiseconds,
                            const struct port_exe_power_ops *ops) {
    if(!processes || !down_centiseconds || !ops || !ops->queue_special_ast)
        return 0u;
    uint32_t downtime=*down_centiseconds;
    unsigned enqueued=0u;
    /* EXE$POWERAST walks SCH$GL_PCBVEC from its highest process index. */
    for(unsigned i=process_slots;i>0u;) {
        struct port_exe_power_process *process=processes[--i];
        if(!process || !process->power_pending)continue;
        uint32_t s=ops->queue_special_ast(ops->context,process->pid,downtime);
        if(s&1u) {
            process->power_pending=0;
            ++enqueued;
        }
    }
    *down_centiseconds=0;
    return enqueued;
}
uint32_t port_exe_powerast_requeue(struct port_exe_power_process *process,
                                   uint32_t down_centiseconds,
                                   const struct port_exe_power_ops *ops) {
    if(!process || !ops || !ops->queue_user_ast)return PORT_SS_BADPARAM;
    /* The kernel-mode AST becomes a normal AST at the saved access mode. */
    uint32_t status=ops->queue_user_ast(ops->context,process->pid,
                         process->ast_address,down_centiseconds,
                         process->ast_mode);
    if(status&1u)process->ast_address=0;
    return status;
}
