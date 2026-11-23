#include "vms_iopost_end.h"
/* Original IOC$IOPOST loops until REMQUE returns empty. A bounded
 * entry count prevents attempts to service any later non-bootstrap IRP. */
uint32_t port_v37_iopost_drain(struct port_v37_iopost_queue *q,
    uint32_t kernel_begin,uint32_t kernel_limit,uint32_t expected_action,
    struct port_v37_iopost_endlog *log,uint32_t max_packets){
    if(!q||!log||!max_packets||max_packets>PORT_V37_IOPOST_LOG_CAP)
        return PORT_V37_IOPOST_BAD;
    for(uint32_t n=0;n<max_packets;n++){
        const uint32_t r=port_v37_iopost_service(q,kernel_begin,
            kernel_limit,expected_action,log);
        if(r!=PORT_V37_IOPOST_OK)return r;
    }
    /* Source IOPOST finishes only when its queue is really empty. */
    uint32_t addr=0;const uint8_t *packet=0;
    if(port_v37_iopost_peek(q,&addr,&packet)!=PORT_V37_IOPOST_EMPTY)
        return PORT_V37_IOPOST_CORRUPT;
    return PORT_V37_IOPOST_OK;
}
