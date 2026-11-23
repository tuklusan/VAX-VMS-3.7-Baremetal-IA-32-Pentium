#include "vms_exe_wait.h"
#include <stddef.h>
static uint32_t wait_common(struct port_exe_wait_process *p,uint32_t efn,
                            uint32_t mask,uint8_t all,
                            const struct port_exe_wait_ops *ops) {
    if(!p || !p->events || !ops || !ops->lock || !ops->unlock ||
       !ops->sleep_locked)return PORT_SS_BADPARAM;
    volatile uint32_t *flags=NULL;
    uint32_t status=port_exe_getefc(p->events,efn&255u,&flags);
    if(!(status&1u))return status;
    p->wait_cluster=(uint8_t)((efn>>5)&3u);
    for(;;) {
        ops->lock(ops->context);
        /* Event flags are bit masks, not promises from the scheduler. */
        uint32_t bits=__atomic_load_n(flags,__ATOMIC_SEQ_CST)&mask;
        if(all?bits==mask:bits!=0u) {
            p->waiting=0;
            ops->unlock(ops->context);
            return PORT_SS_NORMAL;
        }
        p->wait_all=all;
        p->inverted_wait_mask=~mask;
        p->waiting=1;
        if(p->direct_io_available==p->direct_io_limit &&
           p->buffered_io_available!=p->buffered_io_limit &&
           ops->wake_swapper)ops->wake_swapper(ops->context);
        if(p->pending_ast_mode!=4u && p->pending_ast_mode<=p->previous_mode &&
           !p->interrupt_level && ops->report_ast)
            ops->report_ast(ops->context,p);
        /* sleep_locked must enqueue the process and release the lock
           before scheduling; it reacquires the lock on return. */
        status=ops->sleep_locked(ops->context,p);
        p->waiting=0;
        ops->unlock(ops->context);
        if(!(status&1u))return status;
    }
}
uint32_t port_exe_waitfr(struct port_exe_wait_process *p,uint32_t efn,
                        const struct port_exe_wait_ops *ops) {
    return wait_common(p,efn,1u<<(efn&31u),0u,ops);
}
uint32_t port_exe_wflor(struct port_exe_wait_process *p,uint32_t efn,
                       uint32_t mask,const struct port_exe_wait_ops *ops) {
    return wait_common(p,efn,mask,0u,ops);
}
uint32_t port_exe_wfland(struct port_exe_wait_process *p,uint32_t efn,
                        uint32_t mask,const struct port_exe_wait_ops *ops) {
    return wait_common(p,efn,mask,1u,ops);
}
