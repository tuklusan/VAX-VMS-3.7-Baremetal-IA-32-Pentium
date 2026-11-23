#ifndef VMS37_NATIVE_NL_DDT_H
#define VMS37_NATIVE_NL_DDT_H
#include <stdint.h>
#include <stddef.h>
#include "vms_qio_post.h"
#include "vms_ioc_phys.h"
/* SHRLIB/SYSMAR.MAR 216-241 DDTAB, 503-546 FUNCTAB/GENRADDR;
 * SYS/SRC/NLDRIVER.MAR 48-96, 108-133. Absolute native IOC$RETURN
 * address in unused DDT slots; FDT uses relative entries. MNTVER is
 * unported (zero), so this binds ONLY the source null read/write path. */
enum { PORT_V37_NL_DDT_BYTES=38u,PORT_V37_NL_FDT_OFFSET=40u,
       PORT_V37_NL_FDT_BYTES=40u,PORT_V37_NL_IMAGE_BYTES=80u,
       PORT_V37_NL_DDT_OK=1u,PORT_V37_NL_DDT_BAD=2u };
typedef uint32_t (*port_v37_nl_fdt_action)(struct port_v37_qio_request *);
uint32_t port_v37_nl_ddt_build(uint8_t *,size_t,uint32_t,uint32_t,uint32_t,uint32_t);
uint32_t port_v37_nl_ddt_check(const uint8_t *,size_t,uint32_t,uint32_t,uint32_t,uint32_t);
uint32_t port_v37_nl_ddt_masks(uint8_t *,size_t,uint32_t,uint32_t,uint32_t);
uint32_t port_v37_nl_ddt_bind(uint8_t *,size_t,uint32_t,
    struct port_v37_device_result *,uint32_t);
uint32_t port_v37_nl_ddt_dispatch(const uint8_t *,size_t,uint32_t,
    uint32_t,uint32_t,uint32_t,uint16_t,struct port_v37_qio_request *,
    port_v37_nl_fdt_action,port_v37_nl_fdt_action);
uint32_t port_v37_nl_ddt_source_lookup(const uint8_t *,size_t,uint32_t,
    const struct port_v37_device_result *,const char *,size_t,
    struct port_v37_ioc_result *);
uint32_t port_v37_nl_fdt_read(struct port_v37_qio_request *);
uint32_t port_v37_nl_fdt_write(struct port_v37_qio_request *);
uint32_t port_v37_nl_ioc_return(void);
#endif
