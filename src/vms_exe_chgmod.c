#include "vms_exe_chgmod.h"
#include <stddef.h>
static uint32_t change(const struct port_exe_mode_context *process,
                       uint32_t routine,uint32_t arglist,uint8_t target,
                       const struct port_exe_mode_executor *ops) {
    if(!process || !ops || !ops->call_mode)return PORT_SS_BADPARAM;
    /* The original CMEXEC route accepts CMKRNL permission as a fallback. */
    if(target==1u && process->has_cmexec)
        return ops->call_mode(ops->context,1u,routine,arglist);
    if((process->previous_mode&3u)<2u || process->has_cmkrnl)
        return ops->call_mode(ops->context,target,routine,arglist);
    return PORT_SS_NOPRIV;
}
uint32_t port_exe_cmexec(const struct port_exe_mode_context *p,
                         uint32_t routine,uint32_t arglist,
                         const struct port_exe_mode_executor *ops) {
    return change(p,routine,arglist,1u,ops);
}
uint32_t port_exe_cmkrnl(const struct port_exe_mode_context *p,
                         uint32_t routine,uint32_t arglist,
                         const struct port_exe_mode_executor *ops) {
    return change(p,routine,arglist,0u,ops);
}
