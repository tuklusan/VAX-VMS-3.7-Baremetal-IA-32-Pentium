#include "vms_exe_adjstk.h"
#include <stddef.h>
uint32_t port_exe_adjstk(struct port_exe_outer_stacks *stack,uint32_t mode,
                         uint32_t adjust,uint32_t result_address,
                         const struct port_exe_adjust_stack_ops *ops) {
    if(!stack || !ops)return PORT_SS_BADPARAM;
    uint8_t target=(uint8_t)(mode&3u);
    if((stack->previous_mode&3u)>=target)return PORT_SS_NOPRIV;
    if(!port_exe_probe_write(ops->user,result_address,4u) ||
       !port_exe_probe_read(ops->user,result_address,4u))
        return PORT_SS_ACCVIO;
    uint32_t old;
    if(!port_exe_read(ops->user,result_address,&old,4u))
        return PORT_SS_ACCVIO;
    if(!old)old=stack->mode_sp[target];
    int32_t delta=(int16_t)adjust;
    uint32_t new_sp=old+(uint32_t)delta;
    if(delta<0) {
        uint32_t remaining=(uint32_t)(-delta);
        if(new_sp>old)return PORT_SS_ACCVIO;
        if(!ops->probe_growth)return PORT_SS_ACCVIO;
        /* VAX IFNOWRT tests up to one 512-byte page each iteration. */
        uint32_t address=new_sp;
        while(remaining) {
            uint32_t length=remaining>512u?512u:remaining;
            if(!ops->probe_growth(ops->context,target,address,length)) {
                if(target!=3u || !ops->expand_user_stack)return PORT_SS_ACCVIO;
                uint32_t status=ops->expand_user_stack(ops->context,address);
                if(!(status&1u))return status;
                if(!ops->probe_growth(ops->context,target,address,length))
                    return PORT_SS_ACCVIO;
            }
            address+=length;
            remaining-=length;
        }
    }
    if(!port_exe_write(ops->user,result_address,&new_sp,4u))
        return PORT_SS_ACCVIO;
    stack->mode_sp[target]=new_sp;
    return PORT_SS_NORMAL;
}
