#include "vms_exe_event.h"

void port_exe_event_init(struct port_exe_event_process *process,uint32_t pid) {
    if(!process)return;
    process->pid=pid;
    for(unsigned i=0;i<2u;i++) {
        process->local[i]=0u;
        process->common[i]=0;
    }
}
uint32_t port_exe_getefc(struct port_exe_event_process *process,
                         uint32_t efn,volatile uint32_t **mask) {
    if(!process||!mask)return PORT_SS_ILLEFC;
    *mask=0;
    /* VAX MOVZBL on the service argument reduces it to one byte. */
    efn&=255u;
    uint32_t cluster=efn>>5;
    if(cluster>=4u)return PORT_SS_ILLEFC;
    if(cluster<2u) {
        *mask=&process->local[cluster];
        return PORT_SS_NORMAL;
    }
    struct port_exe_common_event *block=process->common[cluster-2u];
    if(!block)return PORT_SS_UNASEFC;
    if(block->slave) {
        block=block->master;
        if(!block)return PORT_SS_UNASEFC;
    }
    *mask=&block->mask;
    return PORT_SS_NORMAL;
}
uint32_t port_exe_clref(struct port_exe_event_process *process,uint32_t efn) {
    volatile uint32_t *bits=0;
    uint32_t status=port_exe_getefc(process,efn,&bits);
    if(!(status&1u))return status;
    uint32_t old=__atomic_fetch_and(bits,~(UINT32_C(1)<<(efn&31u)),__ATOMIC_SEQ_CST);
    return (old&(UINT32_C(1)<<(efn&31u)))?PORT_SS_WASSET:PORT_SS_WASCLR;
}
uint32_t port_exe_readef(struct port_exe_event_process *process,uint32_t efn,
                         uint32_t result_address,port_exe_event_write write,
                         void *context) {
    volatile uint32_t *bits=0;
    uint32_t status=port_exe_getefc(process,efn,&bits);
    if(!(status&1u))return status;
    if(!write)return PORT_SS_ACCVIO;
    uint32_t value=__atomic_load_n(bits,__ATOMIC_SEQ_CST);
    if(!write(context,result_address,&value))return PORT_SS_ACCVIO;
    return (value&(UINT32_C(1)<<(efn&31u)))?PORT_SS_WASSET:PORT_SS_WASCLR;
}
uint32_t port_exe_event_post(struct port_exe_event_process *process,
                             uint32_t efn) {
    volatile uint32_t *bits=0;
    uint32_t status=port_exe_getefc(process,efn,&bits);
    if(!(status&1u))return status;
    uint32_t old=__atomic_fetch_or(bits,UINT32_C(1)<<(efn&31u),__ATOMIC_SEQ_CST);
    return (old&(UINT32_C(1)<<(efn&31u)))?PORT_SS_WASSET:PORT_SS_WASCLR;
}
uint32_t port_exe_setef(struct port_exe_event_process *process,uint32_t efn,
                        uint32_t io_priority,port_exe_event_post_fn post,
                        void *context) {
    if(!process)return PORT_SS_ILLEFC;
    /* The original delegates to SCH$POSTEF, which may reschedule a
     * waiting process. A missing scheduler cannot report success. */
    if(!post)return PORT_SS_BADQUEUEHDR;
    return post(context,process->pid,efn&255u,io_priority);
}
