#ifndef VMS37_NATIVE_DDTRELOC_H
#define VMS37_NATIVE_DDTRELOC_H
#include <stddef.h>
#include <stdint.h>
/* IOC$RELOC_DDT, SYS/SRC/RELOCDRV.MAR 289-349. Strictly offline:
 * this code rewrites a private DDT/FDT image and never binds a driver. */
enum {PORT_DDTRELOC_OK=1u,PORT_DDTRELOC_BAD=2u,
      PORT_DDTRELOC_UNBOUND=4u,PORT_DDTRELOC_DDT_LEN=38u,
      PORT_DDTRELOC_FDT_OFF=8u,PORT_DDTRELOC_FDT_SIZE_OFF=36u,
      PORT_DDTRELOC_FDT_START=40u};
struct port_ddtreloc_image {
    uint8_t *image;
    size_t length;
    uint32_t pc_base;
};
uint16_t port_ddtreloc_read16(const uint8_t *p);
uint32_t port_ddtreloc_read32(const uint8_t *p);
void port_ddtreloc_write32(uint8_t *p,uint32_t v);
uint32_t port_ddtreloc_preflight(const struct port_ddtreloc_image *im,
    uint32_t *fdt_offset,uint16_t *fdt_length);
uint32_t port_ddtreloc_header(const struct port_ddtreloc_image *im);
uint32_t port_ddtreloc_functions(const struct port_ddtreloc_image *im,
    uint32_t fdt_offset,uint16_t fdt_length);
uint32_t port_ddtreloc_apply(const struct port_ddtreloc_image *im);
#endif
