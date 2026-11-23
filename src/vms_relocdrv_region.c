#include "vms_relocdrv.h"
/* Source LOCATE: CRB,DDB,IDB,UCB. Never chase unchecked VAX pointers,
 * especially UCB$L_CRB; the caller supplies explicit verified regions. */
uint32_t port_relocdrv_region(const struct port_relocdrv_context *c,
    uint8_t type,unsigned length,uint8_t **out){
    if(!c||!out||!length||length>4u)return PORT_RELOCDRV_BAD;
    if(type!=PORT_V37_DYN_CRB&&type!=PORT_V37_DYN_DDB&&
       type!=PORT_V37_DYN_IDB&&type!=PORT_V37_DYN_UCB)
        return PORT_RELOCDRV_BAD;
    for(unsigned i=0;i<PORT_RELOCDRV_REGION_COUNT;i++){
        const struct port_relocdrv_region *r=&c->region[i];
        if(r->structure_type!=type)continue;
        if(!r->bytes||r->bytes_size<length||!r->pc_base||
           (uint64_t)r->pc_base+r->bytes_size>UINT32_MAX)
            return PORT_RELOCDRV_UNBOUND;
        *out=r->bytes;return PORT_RELOCDRV_OK;
    }
    return PORT_RELOCDRV_UNBOUND;
}
