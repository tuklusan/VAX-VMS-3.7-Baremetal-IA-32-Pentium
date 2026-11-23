#include "vms_exe_cancel.h"
#include <stddef.h>
uint32_t port_exe_canceln(struct port_exe_io_channel *ch,uint32_t caller,
                          uint8_t reason,const struct port_exe_io_ops *ops) {
    if(!ch || !ops || !ops->driver_cancel || !ops->post_io || !ops->lock ||
       !ops->unlock || !ch->assigned || !ch->device || ch->owner_pid!=caller)
        return PORT_SS_BADPARAM;
    ops->lock(ops->context);
    struct port_exe_io_request **ptr=&ch->requests;
    while(*ptr){
        struct port_exe_io_request *r=*ptr;
        if(r->pid!=caller || r->channel!=ch->channel || r->virtual_io || !r->queued){
            ptr=&r->next;continue;
        }
        *ptr=r->next;
        r->next=NULL;r->queued=0;r->status=PORT_SS_CANCEL;
        if(r->buffered)r->buffered=0;
        uint32_t st=ops->post_io(ops->context,r);
        if(!(st&1u)) {ops->unlock(ops->context);return st;}
        /* Completion processing, rather than CANCEL, returns I/O quota. */
    }
    uint32_t status=ops->driver_cancel(ops->context,ch->device,ch->channel,reason);
    ops->unlock(ops->context);
    return status;
}
uint32_t port_exe_cancel(struct port_exe_io_channel *ch,uint32_t caller,
                         const struct port_exe_io_ops *ops) {
    return port_exe_canceln(ch,caller,PORT_EXE_CANCEL_NORMAL,ops);
}
