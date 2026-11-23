#include "vms_exe_setstk.h"
#include <stddef.h>
uint32_t port_exe_setstk(struct port_exe_stack_bounds *stack,
                         uint32_t input,uint32_t previous,uint32_t mode,
                         const struct port_exe_access *user) {
    if(!stack)return PORT_SS_BADPARAM;
    unsigned target=mode&3u;
    if(target<(stack->previous_mode&3u))target=stack->previous_mode&3u;
    /* The original treats user-stack limit changes as an unconditional no-op. */
    if(target==3u)return PORT_SS_NORMAL;
    if(!input || !port_exe_probe_read(user,input,8u) ||
       (previous && !port_exe_probe_write(user,previous,8u)))
        return PORT_SS_ACCVIO;
    if(previous) {
        uint32_t old[2]={stack->low[target],stack->high[target]};
        if(!port_exe_write(user,previous,old,8u))return PORT_SS_ACCVIO;
    }
    uint32_t limits[2];
    if(!port_exe_read(user,input,limits,8u))return PORT_SS_ACCVIO;
    stack->low[target]=limits[0];
    stack->high[target]=limits[1];
    return PORT_SS_NORMAL;
}
