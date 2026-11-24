#ifndef VMS37_SYSQIO_PENDING_H
#define VMS37_SYSQIO_PENDING_H
#include "vms_iopost_end.h"
#include "vms_sysqio_args.h"

#define PORT_V37_SYSQIO_PENDING_DEPTH 32u
enum { PORT_V37_SYSQIO_PENDING_OK=1u,PORT_V37_SYSQIO_PENDING_BAD=2u,
       PORT_V37_SYSQIO_PENDING_FULL=4u,PORT_V37_SYSQIO_PENDING_DUP=6u,
       PORT_V37_SYSQIO_PENDING_NOTFOUND=8u };
struct port_v37_sysqio_pending_entry {
    struct port_v37_sysqio_prepared request;
    uint32_t pid,irp_va;
    uint8_t request_mode,state;
};
struct port_v37_sysqio_pending {
    struct port_v37_sysqio_pending_entry entries[
        PORT_V37_SYSQIO_PENDING_DEPTH];
    struct port_v37_irp_pool_sync sync;
};
void port_v37_sysqio_pending_init(struct port_v37_sysqio_pending *,
    const struct port_v37_irp_pool_sync *);
/* On success, transfer page-pin ownership moves from prepared to the table. */
uint32_t port_v37_sysqio_pending_track(struct port_v37_sysqio_pending *,
    uint32_t,uint32_t,const uint8_t *,size_t,
    struct port_v37_sysqio_prepared *);
/* Roll back a request whose DDT submission failed before it became active.
 * Only a tracked, not-yet-completing owner can release the retained pins. */
uint32_t port_v37_sysqio_pending_abort(struct port_v37_sysqio_pending *,
    uint32_t,uint32_t,uint32_t,uint8_t);
/* These callbacks bind directly to DIRPOST's optional retained-QIO path. */
int port_v37_sysqio_pending_validate(void *,uint32_t,uint32_t,uint32_t,
    uint8_t);
uint32_t port_v37_sysqio_pending_complete(void *,uint32_t,uint32_t,uint32_t,
    uint8_t,uint32_t,uint32_t);
#endif
