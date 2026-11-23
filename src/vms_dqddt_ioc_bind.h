#ifndef VMS37_DQDDT_IOC_BIND_H
#define VMS37_DQDDT_IOC_BIND_H
#include <stddef.h>
#include <stdint.h>
#include "vms_dqddt.h"
#include "vms_dq_crb.h"
enum {PORT_DQDDT_BIND_OK=1u,PORT_DQDDT_BIND_BAD=2u};
uint32_t port_dqddt_publish_ioc(uint8_t *,size_t,uint32_t,
    struct port_v37_device_result *,const struct port_dqddt_stage *,
    const struct port_dq_fdt *,const struct port_dqddt_native *,
    const struct port_dq_crb *);
#endif
