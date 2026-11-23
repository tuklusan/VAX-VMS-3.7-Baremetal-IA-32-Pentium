#include "vms_iopost_end.h"
/* IOCIOPOST.MAR 149-154: REMQUE @IOC$GL_PSFL,R5 reads FIFO head.
 * Validate before reading packet bytes, then defer state mutation. */
uint32_t port_v37_iopost_peek(const struct port_v37_iopost_queue *q,
    uint32_t *address,const uint8_t **packet){
    if(!q||!address||!packet)return PORT_V37_IOPOST_BAD;
    if(port_v37_post_validate(q)!=PORT_V37_QIO_OK)
        return PORT_V37_IOPOST_CORRUPT;
    if(!q->queued)return PORT_V37_IOPOST_EMPTY;
    *address=q->first;
    *packet=q->bytes+(q->first-q->first_irp);
    return PORT_V37_IOPOST_OK;
}
