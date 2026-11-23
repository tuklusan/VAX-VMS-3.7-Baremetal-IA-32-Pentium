#include "vms_sysqio_args.h"
#include "vms_exe_schevt.h"
#include "vms_sys85.h"
#include "vms_v37_pcb.h"

static uint32_t load32(const uint8_t *p){
    return (uint32_t)p[0]|((uint32_t)p[1]<<8)|
           ((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);
}
static uint32_t clear_process_efn(struct port_v37_sysqio_process *process,
    uint32_t efn,const struct port_v37_sysqio_begin_ops *ops,
    uint32_t *service_status){
    if(!process->source_pcb||efn>63u)
        return port_v37_sysqio_clear_efn(efn,process->local_event_flags,
            ops->clear_global_efn,ops->context,service_status);
    const unsigned cluster=efn>>5u;
    const uint32_t off=cluster?PORT_V37_PCB_EFCU:PORT_V37_PCB_EFCS;
    uint32_t *const flags=(uint32_t *)(void *)(process->source_pcb+off);
    (void)__atomic_fetch_and(flags,~(UINT32_C(1)<<(efn&31u)),
                             __ATOMIC_SEQ_CST);
    *service_status=PORT_SYS85_NORMAL;
    return PORT_V37_SYSQIO_EF_OK;
}

/* SYS/SRC/SYSQIOREQ.MAR 239-272 and 286-290. The source entry reads the
 * argument count at AP+0 and uses fixed longword slots through P6 at AP+48;
 * FUNC is a word value and is zero-extended by the service. */
uint32_t port_v37_sysqio_decode_args(const void *argument_list,
    size_t accessible_bytes,struct port_v37_sysqio_args *decoded){
    if(!argument_list||!decoded||((uintptr_t)argument_list&3u)||
       accessible_bytes<PORT_V37_SYSQIO_LIST_BYTES)
        return PORT_V37_SYSQIO_ARGS_BAD;
    const uint8_t *const ap=argument_list;
    if(load32(ap)<PORT_V37_SYSQIO_REQUIRED_ARGS)
        return PORT_V37_SYSQIO_ARGS_BAD;
    struct port_v37_sysqio_args out={0};
    out.event_flag=load32(ap+4u);
    out.channel=load32(ap+8u);
    out.function=(uint16_t)load32(ap+12u);
    out.iosb=load32(ap+16u);
    out.ast_address=load32(ap+20u);
    out.ast_parameter=load32(ap+24u);
    for(unsigned i=0;i<6u;i++)out.parameter[i]=load32(ap+28u+4u*i);
    *decoded=out;
    return PORT_V37_SYSQIO_ARGS_OK;
}

/* SYSQIOREQ.MAR 253-283. CCB$B_AMOD is MAXACMODE+1 as stored by SYS$ASSIGN;
 * the strict comparison therefore admits a request from the assigning mode
 * while rejecting access from a less privileged mode. */
uint32_t port_v37_sysqio_preflight(const struct port_v37_sysqio_args *args,
    uint8_t caller_mode,uint8_t ccb_access_limit,
    const struct port_v37_sysqio_device *device,
    struct port_v37_sysqio_dispatch *dispatch){
    if(!args||!device||!dispatch||caller_mode>3u||
       !device->fdt||!device->fdt_bytes||
       ccb_access_limit==0u||ccb_access_limit>4u||
       device->online>1u)
        return PORT_V37_SYSQIO_PRECHECK_BAD;
    if(caller_mode>=ccb_access_limit)
        return PORT_V37_SYSQIO_PRECHECK_NOPRIV;
    struct port_dq_fdt_decision fdt={0};
    if(port_dq_fdt_classify(device->fdt,device->fdt_bytes,args->function,
                            &fdt)!=PORT_DQ_FDT_OK)
        return PORT_V37_SYSQIO_PRECHECK_ILLFUNC;
    if(!device->online&&fdt.number!=PORT_DQ_DEACCESS&&
       fdt.number!=PORT_DQ_ACPCONTROL)
        return PORT_V37_SYSQIO_PRECHECK_OFFLINE;
    const struct port_v37_sysqio_dispatch out={
        .full_function=args->function,.function_code=fdt.number,
        .buffered=fdt.buffered,.action_class=fdt.action_class};
    *dispatch=out;
    return PORT_V37_SYSQIO_PRECHECK_OK;
}

static uint32_t begin_failure(uint32_t result,uint32_t status,
    const struct port_v37_sysqio_args *args,
    const struct port_v37_sysqio_process *process,
    const struct port_v37_sysqio_begin_ops *ops,uint32_t *out_status){
    *out_status=status;
    if(ops->post_event)
        ops->post_event(ops->context,process->pid,
                        (uint8_t)args->event_flag,status);
    return result;
}

/* SYSQIOREQ.MAR NOIOSB/NOCNT: charge the per-process direct or buffered
 * counter. A negative post-decrement is admitted only when the executive's
 * single-unit quota service grants the request; otherwise the charge is
 * backed out exactly as the source does before posting SS$_EXQUOTA. */
static int charge_io_quota(struct port_v37_sysqio_process *process,
    const struct port_v37_sysqio_begin_ops *ops,uint8_t buffered){
    const int16_t before=port_v37_sysqio_process_io_count(process,buffered);
    const int32_t next=(int32_t)before-1;
    if(next>=0){port_v37_sysqio_process_set_io_count(process,buffered,
                                                     (int16_t)next);return 1;}
    if(ops->single_io_quota&&
       ops->single_io_quota(ops->context,process->pid,buffered)&&
       next>=INT16_MIN){
        port_v37_sysqio_process_set_io_count(process,buffered,(int16_t)next);
        return 1;
    }
    return 0;
}

/* Use an explicitly supplied compatibility hook for host fixtures, or the
 * source CTL$GL_CCBBASE index and UCB resolver on the native service path. */
static uint32_t resolve_channel(const struct port_v37_sysqio_begin_ops *ops,
    uint32_t pid,uint32_t token,uint8_t mode,
    struct port_v37_sysqio_resolved_channel *out){
    if(ops->lookup_channel)
        return ops->lookup_channel(ops->context,pid,token,out)==0&&
            out->device&&out->ucb?PORT_V37_SYSQIO_BEGIN_OK:
                                  PORT_V37_SYSQIO_BEGIN_IVCHAN;
    if(!ops->ccb_base||!ops->lookup_ucb||
       !ops->ccb_maximum_channel_offset)
        return PORT_V37_SYSQIO_BEGIN_IVCHAN;
    struct port_v37_ccb_view view={0};
    const uint32_t status=port_v37_ccb_resolve(ops->ccb_base,
        ops->ccb_preceding_bytes,token,ops->ccb_maximum_channel_offset,
        mode,&view);
    if(status==PORT_V37_CCB_NOPRIV)return PORT_V37_SYSQIO_BEGIN_NOPRIV;
    if(status!=PORT_V37_CCB_OK)return PORT_V37_SYSQIO_BEGIN_IVCHAN;
    struct port_v37_sysqio_resolved_channel resolved={0};
    if(ops->lookup_ucb(ops->context,pid,view.ucb,&resolved)!=0||
       !resolved.device||!resolved.ucb)
        return PORT_V37_SYSQIO_BEGIN_IVCHAN;
    resolved.window=view.window;
    resolved.ccb_access_limit=view.access_limit;
    *out=resolved;
    return PORT_V37_SYSQIO_BEGIN_OK;
}

/* SYSQIOREQ.MAR 239-292. This is the actual source-ordered service front end
 * through IOSB initialization. Driver FDT invocation and IRP submission are
 * returned to the caller as the next, still-unported stage. */
uint32_t port_v37_sysqio_begin(const void *ap,size_t ap_bytes,
    struct port_v37_sysqio_process *process,
    const struct port_v37_sysqio_begin_ops *ops,
    struct port_v37_sysqio_prepared *prepared,uint32_t *service_status){
    if(!process||!process->pid||process->access_mode>3u||!ops||
       (!ops->lookup_channel&&(!ops->ccb_base||!ops->lookup_ucb||
          !ops->ccb_maximum_channel_offset))||!ops->post_event||
       !prepared||!service_status)
        return PORT_V37_SYSQIO_BEGIN_BAD;
    struct port_v37_sysqio_args args={0};
    if(port_v37_sysqio_decode_args(ap,ap_bytes,&args)!=PORT_V37_SYSQIO_ARGS_OK){
        *service_status=PORT_SYS85_BADPARAM;
        return PORT_V37_SYSQIO_BEGIN_BAD;
    }
    uint32_t efn_status=0u;
    const uint32_t efn=clear_process_efn(process,args.event_flag,ops,
                                          &efn_status);
    if(efn!=PORT_V37_SYSQIO_EF_OK)
        return begin_failure(PORT_V37_SYSQIO_BEGIN_EFERR,efn_status,
                             &args,process,ops,service_status);
    struct port_v37_sysqio_resolved_channel channel={0};
    uint32_t lookup=resolve_channel(ops,process->pid,args.channel,
                                    process->access_mode,&channel);
    if(lookup!=PORT_V37_SYSQIO_BEGIN_OK){
        const uint32_t status=lookup==PORT_V37_SYSQIO_BEGIN_NOPRIV?
            PORT_SYS85_NOPRIV:PORT_SYS85_IVCHAN;
        return begin_failure(lookup,status,&args,process,ops,service_status);
    }
    /* SYSQIOREQ.MAR DACSPND waits on SCH$GQ_MWAIT with RSN$_ASTWAIT while
     * access/deaccess owns CCB$L_WIND bit 0. Never dispatch through a CCB in
     * that transient state; callers without a scheduler binding must retry. */
    if(channel.window&1u){
        if(!ops->wait_channel)return PORT_V37_SYSQIO_BEGIN_WAIT;
        ops->wait_channel(ops->context,process->pid,args.channel,&channel);
        if(!ops->lookup_channel){
            lookup=resolve_channel(ops,process->pid,args.channel,
                                   process->access_mode,&channel);
            if(lookup!=PORT_V37_SYSQIO_BEGIN_OK){
                const uint32_t status=lookup==PORT_V37_SYSQIO_BEGIN_NOPRIV?
                    PORT_SYS85_NOPRIV:PORT_SYS85_IVCHAN;
                return begin_failure(lookup,status,&args,process,ops,
                                     service_status);
            }
        }
        if((channel.window&1u)||!channel.device||!channel.ucb)
            return PORT_V37_SYSQIO_BEGIN_WAIT;
    }
    struct port_v37_sysqio_dispatch dispatch={0};
    const uint32_t check=port_v37_sysqio_preflight(&args,
        process->access_mode,channel.ccb_access_limit,channel.device,&dispatch);
    if(check!=PORT_V37_SYSQIO_PRECHECK_OK){
        const uint32_t result=check==PORT_V37_SYSQIO_PRECHECK_NOPRIV?
            PORT_V37_SYSQIO_BEGIN_NOPRIV:
            check==PORT_V37_SYSQIO_PRECHECK_ILLFUNC?
                PORT_V37_SYSQIO_BEGIN_ILLFUNC:
            check==PORT_V37_SYSQIO_PRECHECK_OFFLINE?
                PORT_V37_SYSQIO_BEGIN_OFFLINE:PORT_V37_SYSQIO_BEGIN_BAD;
        const uint32_t status=check==PORT_V37_SYSQIO_PRECHECK_NOPRIV?
            PORT_SYS85_NOPRIV:
            check==PORT_V37_SYSQIO_PRECHECK_ILLFUNC?
                PORT_SYS85_ILLIOFUNC:
            check==PORT_V37_SYSQIO_PRECHECK_OFFLINE?
                PORT_SYS85_DEVOFFLINE:PORT_SYS85_BADPARAM;
        return begin_failure(result,status,&args,process,ops,service_status);
    }
    const uint32_t iosb=port_v37_sysqio_clear_iosb(args.iosb,
                                                   &ops->user_memory);
    if(iosb!=PORT_V37_SYSQIO_IOSB_OK){
        const uint32_t result=iosb==PORT_V37_SYSQIO_IOSB_ACCVIO?
            PORT_V37_SYSQIO_BEGIN_ACCVIO:PORT_V37_SYSQIO_BEGIN_BAD;
        const uint32_t status=iosb==PORT_V37_SYSQIO_IOSB_ACCVIO?
            PORT_SYS85_ACCVIO:PORT_SYS85_BADPARAM;
        return begin_failure(result,status,&args,process,ops,service_status);
    }
    if(!charge_io_quota(process,ops,dispatch.buffered))
        return begin_failure(PORT_V37_SYSQIO_BEGIN_EXQUOTA,
            PORT_SS_EXQUOTA,&args,process,ops,service_status);
    const struct port_v37_sysqio_prepared out={args,dispatch,channel};
    *prepared=out;
    *service_status=PORT_V37_SYSQIO_BEGIN_OK;
    return PORT_V37_SYSQIO_BEGIN_OK;
}

