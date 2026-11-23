#include "vms_iopost_end.h"
/* IOCIOPOST.MAR 145-165: remove one source IRP, dispatch only a
 * whitelist-verified kernel end-action, then return to queue loop.
 * PCB, buffer unlock, event flag and kernel AST paths remain blocked. */
uint32_t port_v37_iopost_service(struct port_v37_iopost_queue *q,
    uint32_t kernel_begin,uint32_t kernel_limit,uint32_t expected_action,
    struct port_v37_iopost_endlog *log){
    if(!log || log->completed>=PORT_V37_IOPOST_LOG_CAP)
        return PORT_V37_IOPOST_FULL;
    uint32_t front=port_v37_iopost_validate(q,kernel_begin,
                                              kernel_limit,expected_action);
    if(front!=PORT_V37_IOPOST_OK)return front;
    uint32_t addr=0;const uint8_t *packet=0;
    front=port_v37_iopost_peek(q,&addr,&packet);
    if(front!=PORT_V37_IOPOST_OK)return front;
    const uint32_t status=port_v37_read32(packet+PORT_V37_IRP_MEDIA);
    const uint32_t secondary=port_v37_read32(packet+PORT_V37_IRP_IOST2);
    uint32_t popped=0;
    front=port_v37_iopost_pop(q,&popped);
    if(front!=PORT_V37_IOPOST_OK || popped!=addr)
        return PORT_V37_IOPOST_CORRUPT;
    return port_v37_iopost_kernel_end(log,popped,status,secondary);
}
