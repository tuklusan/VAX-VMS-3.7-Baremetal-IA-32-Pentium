#include "vms_iopost_end.h"
/* IOCIOPOST.MAR 158-159: invoke the explicitly privileged end action
 * and return to IOPOST. This bootstrap action only records the original
 * IRP$L_IOST1/IOST2 and the removed packet; no IOSB/AST is fabricated. */
uint32_t port_v37_iopost_kernel_end(struct port_v37_iopost_endlog *log,
    uint32_t irp_va,uint32_t status,uint32_t secondary){
    if(!log||!irp_va||log->completed>=PORT_V37_IOPOST_LOG_CAP)
        return PORT_V37_IOPOST_FULL;
    const uint32_t n=log->completed;
    log->irp_va[n]=irp_va;
    log->status[n]=status;
    log->secondary[n]=secondary;
    log->completed=n+1u;
    return PORT_V37_IOPOST_OK;
}
