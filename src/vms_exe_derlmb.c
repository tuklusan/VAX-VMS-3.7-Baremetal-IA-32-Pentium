#include "vms_exe_derlmb.h"
#include <stddef.h>
uint32_t port_exe_derlmb(struct port_exe_error_mailbox *mailbox,
                         uint32_t pid,uint32_t unit,int has_diagnose_privilege,
                         const struct port_exe_erlog_ops *ops) {
    if(!mailbox || !ops || !ops->lock || !ops->unlock)
        return PORT_SS_BADPARAM;
    if(!has_diagnose_privilege)return PORT_SS_NOPRIV;
    ops->lock(ops->context);
    uint32_t status=PORT_SS_DEVALLOC;
    if(!mailbox->unit || mailbox->owner_pid==pid) {
        mailbox->unit=(uint16_t)unit;
        mailbox->owner_pid=mailbox->unit?pid:0u;
        status=PORT_SS_NORMAL;
    }
    ops->unlock(ops->context);
    return status;
}
