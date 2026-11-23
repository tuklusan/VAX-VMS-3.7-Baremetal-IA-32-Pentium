#include "vms_ddtreloc.h"
/* VAX little-endian DDT dispatch pointer word. */
uint32_t port_ddtreloc_read32(const uint8_t *p){
    return (uint32_t)p[0]|((uint32_t)p[1]<<8u)|
        ((uint32_t)p[2]<<16u)|((uint32_t)p[3]<<24u);
}
