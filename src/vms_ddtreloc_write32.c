#include "vms_ddtreloc.h"
/* Native IA-32 byte serialization; no unaligned access assumption. */
void port_ddtreloc_write32(uint8_t *p,uint32_t v){
    for(unsigned i=0;i<4u;i++)p[i]=(uint8_t)(v>>(8u*i));
}
