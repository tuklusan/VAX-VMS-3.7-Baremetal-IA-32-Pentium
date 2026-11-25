#ifndef VMS37_SYSQIO_PENDING_H
#define VMS37_SYSQIO_PENDING_H
#include "vms_iopost_end.h"
#include "vms_sysqio_args.h"

#define PORT_V37_SYSQIO_PENDING_DEPTH 32u
enum { PORT_V37_SYSQIO_PENDING_OK=1u,PORT_V37_SYSQIO_PENDING_BAD=2u,
       PORT_V37_SYSQIO_PENDING_FULL=4u,PORT_V37_SYSQIO_PENDING_DUP=6u,
       PORT_V37_SYSQIO_PENDING_NOTFOUND=8u,
       PORT_V37_SYSQIO_PENDING_DEFER=10u };
enum { PORT_V37_SYSQIO_SUBMIT_OK=1u,PORT_V37_SYSQIO_SUBMIT_BAD=2u,
       PORT_V37_SYSQIO_SUBMIT_TRACK=4u,
       PORT_V37_SYSQIO_SUBMIT_REJECTED=6u };
/* The driver callback must return zero only after accepting the request. A
 * nonzero return promises that no completion can race the caller's rollback. */
typedef int (*port_v37_sysqio_driver_submit_fn)(void *,uint32_t,uint32_t,
    uint8_t *,size_t);
struct port_v37_sysqio_pending_entry {
    struct port_v37_sysqio_prepared request;
    uint32_t pid,irp_va;
    uint16_t transfer_bytes;
    uint8_t request_mode,state,transfer_busy,reserved;
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
/* Install process/page-pin ownership before invoking the DDT. Rejected
 * submissions roll back that ownership; accepted requests retain it through
 * DIRPOST completion. */
uint32_t port_v37_sysqio_submit(struct port_v37_sysqio_pending *,uint32_t,
    uint32_t,uint8_t *,size_t,struct port_v37_sysqio_prepared *,
    port_v37_sysqio_driver_submit_fn,void *);
/* Roll back a request whose DDT submission failed before it became active.
 * Only a tracked, not-yet-completing owner can release the retained pins. */
uint32_t port_v37_sysqio_pending_abort(struct port_v37_sysqio_pending *,
    uint32_t,uint32_t,uint32_t,uint8_t);
/* Native DQ staging moves bytes through the retained pinned pages, never
 * through a possibly inactive process VA. Transfers append in order. */
uint32_t port_v37_sysqio_pending_copy_from_user(
    struct port_v37_sysqio_pending *,uint32_t,uint32_t,uint32_t,void *,uint16_t);
uint32_t port_v37_sysqio_pending_copy_to_user(
    struct port_v37_sysqio_pending *,uint32_t,uint32_t,uint32_t,const void *,
    uint16_t);
/* These callbacks bind directly to DIRPOST's optional retained-QIO path. */
int port_v37_sysqio_pending_validate(void *,uint32_t,uint32_t,uint32_t,
    uint8_t,uint32_t);
uint32_t port_v37_sysqio_pending_complete(void *,uint32_t,uint32_t,uint32_t,
    uint8_t,uint32_t,uint32_t);
#endif
