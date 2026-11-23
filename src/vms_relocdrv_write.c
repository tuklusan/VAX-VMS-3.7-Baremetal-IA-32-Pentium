#include "vms_relocdrv.h"
/* RELOCDRV.MAR EXECUTE store byte, word or longword after full preflight. */
void port_relocdrv_write(uint8_t *p,unsigned n,uint32_t v){
    for(unsigned i=0;i<n;i++)p[i]=(uint8_t)(v>>(8u*i));
}
