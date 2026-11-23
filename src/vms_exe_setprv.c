#include "vms_exe_setprv.h"
#include <stddef.h>
uint32_t port_exe_setprv(struct port_exe_privileges *process,
                         const struct port_exe_setprv_request *request,
                         const struct port_exe_access *user) {
    if(!process || !request)return PORT_SS_BADPARAM;
    if(request->permanent_flag&~1u)return PORT_SS_IVSTSFLG;
    uint64_t requested=0;
    if(request->privilege_address &&
       !port_exe_read(user,request->privilege_address,&requested,8u))
        return PORT_SS_ACCVIO;
    uint64_t previous=request->permanent_flag?process->permanent:process->current;
    if(request->old_address &&
       (!port_exe_probe_write(user,request->old_address,8u) ||
        !port_exe_write(user,request->old_address,&previous,8u)))
        return PORT_SS_ACCVIO;
    if(!request->privilege_address)return PORT_SS_NORMAL;
    uint32_t status=PORT_SS_NORMAL;
    if(request->enable_flag&1u) {
        /* Inner modes and SETPRV-authorized processes may set every bit. */
        if(process->previous_mode>1u &&
           !(process->authorized&(UINT64_C(1)<<PORT_PRV_SETPRV_BIT))) {
            uint64_t allowed=process->authorized;
            if(!request->permanent_flag)allowed|=process->image;
            if(requested&~allowed)status=PORT_SS_NOTALLPRIV;
            requested&=allowed;
        }
        process->current|=requested;
        process->process_header|=requested;
        if(request->permanent_flag)process->permanent|=requested;
    } else {
        process->current&=~requested;
        process->process_header&=~requested;
        if(request->permanent_flag)process->permanent&=~requested;
    }
    return status;
}
