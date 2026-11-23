#ifndef VMS37_SYSQIO_IOSB_H
#define VMS37_SYSQIO_IOSB_H
#include <stddef.h>
#include <stdint.h>

enum { PORT_V37_SYSQIO_IOSB_BYTES=8u,
       PORT_V37_SYSQIO_IOSB_OK=1u,
       PORT_V37_SYSQIO_IOSB_BAD=2u,
       PORT_V37_SYSQIO_IOSB_ACCVIO=4u };
typedef int (*port_v37_sysqio_probe_write_fn)(void *,uint32_t,size_t);
/* clear_iosb must clear all eight bytes or leave them untouched. */
typedef int (*port_v37_sysqio_clear_memory_fn)(void *,uint32_t,size_t);
struct port_v37_sysqio_memory {
    port_v37_sysqio_probe_write_fn probe_write;
    port_v37_sysqio_clear_memory_fn clear_memory;
    void *context;
};

uint32_t port_v37_sysqio_clear_iosb(uint32_t iosb,
    const struct port_v37_sysqio_memory *memory);
#endif
