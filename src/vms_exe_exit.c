#include "vms_exe_exit.h"
#include <stddef.h>
uint32_t port_exe_exit(struct port_exe_exit_process *p,uint32_t code,
                       const struct port_exe_exit_ops *ops) {
    if(!p || !p->handlers || !ops || !ops->memory || !ops->invoke ||
       !ops->delete_self)return PORT_SS_BADPARAM;
    p->final_status=code;
    p->force_exit_pending=0;
    unsigned count=0;
    /* The original exits user, supervisor, then executive mode. */
    for(int mode=3;mode>=1;--mode) {
        if((uint8_t)mode > p->previous_mode)continue;
        uint32_t head=p->handlers->head[mode];
        p->handlers->head[mode]=0;
        while(head){
            if(++count>65536u)return PORT_SS_BADPARAM;
            uint32_t data[4];
            if(!port_exe_probe_read(ops->memory,head,sizeof data) ||
               !port_exe_read(ops->memory,head,data,sizeof data))
                return PORT_SS_ACCVIO;
            /* The exit block's status word is at the word reached through
               its fourth longword. The handler is given its call list. */
            if(data[3] && (!port_exe_probe_write(ops->memory,data[3],4u) ||
               !port_exe_write(ops->memory,data[3],&code,4u)))
                return PORT_SS_ACCVIO;
            uint32_t zero=0;
            if(!port_exe_probe_write(ops->memory,head,4u) ||
               !port_exe_write(ops->memory,head,&zero,4u))
                return PORT_SS_ACCVIO;
            uint32_t status=ops->invoke(ops->context,(uint8_t)mode,data[1],data[2]);
            if(!(status&1u))return status;
            head=data[0];
        }
    }
    /* This succeeds only if DELPRC really completes in the host kernel. */
    return ops->delete_self(ops->context,p->pid,code);
}
