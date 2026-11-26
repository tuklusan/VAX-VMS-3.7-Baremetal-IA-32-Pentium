#include "vms_exe_creprc.h"
#include "vms_exe_gettim.h"
#include "vms_exe_setprv.h"

static int has_privilege(uint64_t mask,unsigned bit){
    return bit<64u&&((mask>>bit)&UINT64_C(1))!=0u;
}

uint32_t port_exe_creprc_preflight(
    const struct port_exe_creprc_caller *caller,
    const struct port_exe_creprc_request *request,
    struct port_exe_creprc_result *result){
    if(!caller||!request||!result||caller->base_priority>=32u)
        return PORT_SS_BADPARAM;
    if(request->pid_output_address&&!request->pid_output_writable)
        return PORT_SS_ACCVIO;
    if(request->privilege_address&&!request->privilege_readable)
        return PORT_SS_ACCVIO;
    if(request->uic&&!has_privilege(caller->privilege_mask,
                                   PORT_V37_PRC_PRIV_DETACH))
        return PORT_SS_NOPRIV;
    if(request->status_flags&~(uint32_t)PORT_V37_PRC_STATUS_FLAGS)
        return PORT_SS_IVSTSFLG;

    struct port_exe_creprc_result candidate={0};
    candidate.status_flags=request->status_flags;
    if(request->uic){
        candidate.uic=request->uic;
        candidate.owner_pid=0u;
    }else{
        candidate.uic=caller->uic;
        candidate.owner_pid=caller->pid;
    }
    const uint8_t desired=(uint8_t)(request->base_priority&31u);
    uint8_t internal=(uint8_t)(31u-desired);
    if(!has_privilege(caller->privilege_mask,PORT_V37_PRC_PRIV_SETPRI)&&
       internal<caller->base_priority)
        internal=caller->base_priority;
    candidate.internal_priority=internal;

    uint64_t privileges=request->privilege_address?
        request->requested_privileges:caller->privilege_mask;
    if(request->privilege_address&&
       !has_privilege(caller->privilege_mask,PORT_V37_PRC_PRIV_SETPRV))
        privileges&=caller->privilege_mask;
    candidate.privilege_mask=privileges;
    static const uint8_t required_privilege[9]={
        0xffu,0xffu,PORT_V37_PRC_PRIV_PSWAPM,
        PORT_V37_PRC_PRIV_NOACNT,PORT_V37_PRC_PRIV_DETACH,
        0xffu,PORT_V37_PRC_PRIV_DETACH,PORT_V37_PRC_PRIV_DETACH,0xffu};
    for(unsigned bit=0;bit<9u;++bit){
        const uint8_t required=required_privilege[bit];
        if((request->status_flags&(UINT32_C(1)<<bit))&&required!=0xffu&&
           !has_privilege(candidate.privilege_mask,required))
            return PORT_SS_NOPRIV;
    }
    *result=candidate;
    return PORT_SS_NORMAL;
}
