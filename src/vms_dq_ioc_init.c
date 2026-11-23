#include "vms_dq_ioc.h"
/* M161: construct an empty IOC physical-device queue in supervisor RAM.
 * Never publishes the private device or binds the source DDT. */
uint32_t port_dq_ioc_init(struct port_dq_ioc_queue *q,
                         port_dq_ioc_sector_fn fn,void *context){
    if(!q||!fn)return PORT_DQ_IOC_BAD;
    *q=(struct port_dq_ioc_queue){0};
    q->read_sector=fn;q->context=context;
    q->active=PORT_DQ_IOC_DEPTH;
    return PORT_DQ_IOC_OK;
}
