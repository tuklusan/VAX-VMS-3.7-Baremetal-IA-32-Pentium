#include "vms_exe_canevt.h"
#include <stddef.h>
/* Caller holds the same scheduler lock as EXE$INSTIMQ and time expiry. */
static void credit(struct port_exe_timer_entry *e) {
    if(e->owner) {
        if(e->kind==PORT_EXE_TQE_TMSNGL) {
            if(e->owner->job)++e->owner->job->timer_available;
            if(e->ast)++e->owner->ast_available;
        } else ++e->owner->ast_available;
    }
}
static unsigned remove_matching(struct port_exe_timer_queue *queue,
                                uint32_t pid,uint8_t type,uint32_t request_id,
                                uint8_t minimum_mode) {
    unsigned removed=0;
    for(unsigned i=0;i<PORT_EXE_TQE_CAPACITY;i++) {
        struct port_exe_timer_entry *e=&queue->entries[i];
        if(e->kind==0 || e->target_pid!=pid)continue;
        if(type==PORT_EXE_TQE_TMSNGL) {
            if(e->kind!=type || (request_id && e->ast_parameter!=request_id) ||
               e->mode<minimum_mode)continue;
        } else if(e->kind!=PORT_EXE_TQE_WKSNGL &&
                  e->kind!=PORT_EXE_TQE_WKREPT)continue;
        credit(e);
        e->kind=0;
        --queue->used;
        ++removed;
    }
    return removed;
}
uint32_t port_exe_cantim(struct port_exe_timer_queue *queue,
                         const struct port_exe_timer_process *caller,
                         uint32_t request_id,uint32_t access_mode,
                         unsigned *removed) {
    if(removed)*removed=0;
    if(!queue || !caller)return PORT_SS_BADPARAM;
    /* EXE$MAXACMODE prevents a less privileged mode from cancelling
       a timer created at a more privileged mode. */
    uint8_t mode=(uint8_t)(access_mode&3u);
    if(mode<(caller->previous_mode&3u))mode=caller->previous_mode&3u;
    unsigned n=remove_matching(queue,caller->pid,PORT_EXE_TQE_TMSNGL,
                               request_id,mode);
    if(removed)*removed=n;
    return PORT_SS_NORMAL;
}
uint32_t port_exe_canwak(struct port_exe_timer_queue *queue,
                         const struct port_exe_timer_process *caller,
                         const struct port_exe_cancel_wake_request *request,
                         const struct port_exe_timer_ops *ops,
                         unsigned *removed) {
    if(removed)*removed=0;
    if(!queue || !caller || !request)return PORT_SS_BADPARAM;
    uint32_t target=caller->pid;
    if(request->pid_address || request->logical_name) {
        if(!ops || !ops->resolve_process)return PORT_SS_NONEXPR;
        uint32_t status=ops->resolve_process(ops->context,request->pid_address,
                                      request->logical_name,caller->pid,&target);
        if(!(status&1u))return status;
        if(!target)return PORT_SS_NONEXPR;
    }
    unsigned n=remove_matching(queue,target,PORT_EXE_TQE_WKSNGL,0,0);
    if(removed)*removed=n;
    return PORT_SS_NORMAL;
}
