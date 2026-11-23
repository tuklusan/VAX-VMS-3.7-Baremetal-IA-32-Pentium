#include "vms_ddtreloc.h"
/* Source MOVZWL DDT$W_FDTSIZE, little-endian word. */
uint16_t port_ddtreloc_read16(const uint8_t *p){
    return (uint16_t)((uint16_t)p[0]|((uint16_t)p[1]<<8u));
}
