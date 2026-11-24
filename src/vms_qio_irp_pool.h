#ifndef VMS37_QIO_IRP_POOL_H
#define VMS37_QIO_IRP_POOL_H
#include <stdint.h>
#include "vms_ini_packet_queues.h"
enum {PORT_V37_IRP_POOL_OK=1u,PORT_V37_IRP_POOL_BAD=2u,
      PORT_V37_IRP_POOL_EMPTY=4u,PORT_V37_IRP_POOL_DUP=6u,
      PORT_V37_IRP_POOL_CORRUPT=8u};
struct port_v37_irp_pool_sync {
    void *context;
    uint32_t (*raise_sync)(void *);
    void (*restore_ipl)(void *,uint32_t);
};
/* SYS/SRC/MEMORYALC.MAR ALONONPAGED IRP lookaside and DEANONPAGED return.
 * Caller supplies the original IPL synchronization callbacks; the bounded
 * native pool reports EMPTY where VMS would extend pool or resource-wait. */
uint32_t port_v37_irp_pool_alloc(struct port_ini_packet_list *,uint32_t,
    const struct port_v37_irp_pool_sync *,uint8_t **,uint32_t *);
uint32_t port_v37_irp_pool_free(struct port_ini_packet_list *,uint8_t *,
    uint32_t,uint32_t,const struct port_v37_irp_pool_sync *);
/* MEMORYALC's LRP/SRP lookasides use the same queue discipline but may be
 * smaller than an IRP. Their caller supplies the original DYN type. */
uint32_t port_v37_lookaside_alloc(struct port_ini_packet_list *,uint32_t,
    uint8_t,const struct port_v37_irp_pool_sync *,uint8_t **,uint32_t *);
uint32_t port_v37_lookaside_free(struct port_ini_packet_list *,uint8_t *,
    uint32_t,uint32_t,uint8_t,const struct port_v37_irp_pool_sync *);
#endif
