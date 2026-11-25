#ifndef VMS37_IRP_QUEUE_H
#define VMS37_IRP_QUEUE_H
#include <stddef.h>
#include <stdint.h>

/* SYSQIOREQ.MAR EXE$INSERTIRP and SYSDEF.MDL IRP/UCB I/O queue links.
 * Addresses in the link fields are native 32-bit virtual addresses. */
enum { PORT_V37_UCB_IOQFL=64u, PORT_V37_IRP_IOQFL=0u,
       PORT_V37_IRP_IOQBL=4u, PORT_V37_IRP_PRI=35u,
       PORT_V37_IRP_MIN_BYTES=136u,
       PORT_V37_IRP_QUEUE_OK=1u, PORT_V37_IRP_QUEUE_BAD=2u,
       PORT_V37_IRP_QUEUE_CORRUPT=4u, PORT_V37_IRP_QUEUE_FULL=6u,
       PORT_V37_IRP_QUEUE_NOTFOUND=8u };
typedef int (*port_v37_irp_resolve_fn)(void *,uint32_t,uint8_t **,size_t *);
struct port_v37_irp_queue {
    uint8_t *ucb;
    size_t ucb_bytes;
    uint32_t ucb_va,maximum_entries;
    port_v37_irp_resolve_fn resolve;
    void *context;
};
uint32_t port_v37_irp_queue_init(struct port_v37_irp_queue *,uint8_t *,
    size_t,uint32_t,uint32_t,port_v37_irp_resolve_fn,void *);
uint32_t port_v37_irp_queue_validate(const struct port_v37_irp_queue *,
    uint32_t *);
/* Like EXE$INSERTIRP/REMQUE, callers must hold the device's queue IPL while
 * mutating links. These routines validate but do not raise or restore IPL. */
uint32_t port_v37_irp_queue_insert(struct port_v37_irp_queue *,uint32_t);
uint32_t port_v37_irp_queue_remove(struct port_v37_irp_queue *,uint32_t);
#endif
