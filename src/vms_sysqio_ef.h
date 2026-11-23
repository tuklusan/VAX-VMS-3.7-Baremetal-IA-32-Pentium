#ifndef VMS37_SYSQIO_EF_H
#define VMS37_SYSQIO_EF_H
#include <stdint.h>

enum { PORT_V37_SYSQIO_EF_OK=1u,
       PORT_V37_SYSQIO_EF_BAD=2u,
       PORT_V37_SYSQIO_EF_GLOBAL_FAILURE=4u };
typedef uint32_t (*port_v37_sysqio_clref_fn)(void *,uint8_t);

/* PCB-local EFNs 0-63 clear inline with an atomic longword bit operation;
 * global EFNs use the executive callback. */
uint32_t port_v37_sysqio_clear_efn(uint32_t event_flag,uint32_t local_flags[2],
    port_v37_sysqio_clref_fn clear_global,void *context,
    uint32_t *service_status);
#endif
