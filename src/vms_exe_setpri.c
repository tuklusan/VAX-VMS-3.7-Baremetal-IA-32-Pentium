#include "vms_exe_setpri.h"
#include "vms_exe_schevt.h"
#include <stddef.h>
uint32_t port_exe_setpri(struct port_exe_priority_process *caller,
                         const struct port_exe_setpri_request *request,
                         const struct port_exe_priority_ops *ops) {
    if(!caller || !request || !ops || !ops->reprioritize ||
       (request->old_address && !port_exe_probe_write(ops->memory,
                                                     request->old_address,4u)))
        return request && request->old_address?PORT_SS_ACCVIO:PORT_SS_BADPARAM;
    struct port_exe_priority_process *target=caller;
    if(request->pid_address || request->logical_name) {
        if(!ops->resolve)return PORT_SS_NONEXPR;
        uint32_t status=ops->resolve(ops->context,request->pid_address,
                              request->logical_name,caller->pid,&target);
        if(!(status&1u))return status;
        if(!target)return PORT_SS_NONEXPR;
    }
    /* Priority 0..31 is VMS-facing; internal values count backwards. */
    uint8_t desired=(uint8_t)(31u-(request->priority&31u));
    if(!caller->can_set_any_priority) {
        uint8_t upper=caller->authorized_base;
        if(caller->base<upper)upper=caller->base;
        if(desired<upper)desired=upper;
    }
    uint32_t old=31u-target->saved_base;
    /* IFNOWRT was checked before changing any PCB fields. */
    target->saved_current=desired;
    target->saved_base=desired;
    if(!target->mutex_count) {
        target->base=desired;
        if(target->executing)target->current=desired;
        ops->reprioritize(ops->context,target,desired);
    }
    if(request->old_address &&
       !port_exe_write(ops->memory,request->old_address,&old,4u))
        return PORT_SS_ACCVIO;
    return PORT_SS_NORMAL;
}
