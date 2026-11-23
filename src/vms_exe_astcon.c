#include "vms_exe_astcon.h"
#include <stddef.h>
uint32_t port_exe_setast(struct port_exe_astcon_process *p,uint32_t enabled,
                         const struct port_exe_astcon_ops *ops) {
    if(!p || !ops || !ops->new_ast_level)return PORT_SS_BADPARAM;
    unsigned bit=p->previous_mode&3u;
    uint32_t previous=(p->ast_enabled&(1u<<bit))?PORT_SS_WASSET:PORT_SS_NORMAL;
    if(enabled&1u)p->ast_enabled|=(uint8_t)(1u<<bit);
    else p->ast_enabled&=(uint8_t)~(1u<<bit);
    ops->new_ast_level(ops->context,p);
    return previous;
}
uint32_t port_exe_dclast(struct port_exe_astcon_process *p,
                         uint32_t handler,uint32_t parameter,uint32_t mode,
                         const struct port_exe_astcon_ops *ops) {
    if(!p || !ops || !ops->allocate || !ops->enqueue || !ops->release)
        return PORT_SS_BADPARAM;
    if(!p->ast_available)return PORT_SS_EXQUOTA;
    void *acb=NULL;
    uint32_t status=ops->allocate(ops->context,&acb);
    if(!(status&1u))return status;
    if(!acb)return PORT_SS_INSFMEM;
    --p->ast_available;
    uint8_t access=(uint8_t)(mode&3u);
    if(access<(p->previous_mode&3u))access=p->previous_mode&3u;
    status=ops->enqueue(ops->context,acb,p->pid,handler,parameter,access);
    if(!(status&1u)) {
        ++p->ast_available;
        ops->release(ops->context,acb);
        return status;
    }
    return PORT_SS_NORMAL;
}
