#include "vms_exe_forcex.h"
#include <stddef.h>
uint32_t port_exe_forcex(const struct port_exe_force_process *caller,
                         uint32_t pid_address,uint32_t logical_name,
                         uint32_t code,const struct port_exe_force_ops *ops) {
    if(!caller || !ops || !ops->resolve || !ops->queue_user_exit_ast)
        return PORT_SS_BADPARAM;
    struct port_exe_force_process *target=NULL;
    uint32_t status=ops->resolve(ops->context,pid_address,logical_name,
                                  caller->pid,&target);
    if(!(status&1u))return status;
    if(!target)return PORT_SS_NONEXPR;
    if(target->force_pending)return PORT_SS_NORMAL;
    target->force_pending=1;
    status=ops->queue_user_exit_ast(ops->context,target->pid,code);
    if(!(status&1u)) {
        target->force_pending=0;
        return status;
    }
    return PORT_SS_NORMAL;
}
uint32_t port_exe_forcex_deliver(struct port_exe_force_process *target,
                                 uint32_t code,const struct port_exe_force_ops *ops) {
    if(!target || !ops || !ops->deliver_exit)return PORT_SS_BADPARAM;
    /* CHMK ASTEXIT clears the active AST before SYS$EXIT is called. */
    return ops->deliver_exit(ops->context,target->pid,code);
}
