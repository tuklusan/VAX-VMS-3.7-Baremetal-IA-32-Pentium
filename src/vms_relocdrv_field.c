#include "vms_relocdrv.h"
/* RELOCDRV.MAR INSV performs a native four-byte little-endian bitfield
 * update. Reject cross-boundary masks and shifts before any mutation. */
uint32_t port_relocdrv_field(uint8_t *dst,uint32_t value,unsigned pos,
    unsigned bits,int perform){
    if(!dst||!bits||bits>32u||pos>=32u||pos+bits>32u)
        return PORT_RELOCDRV_BAD;
    if(perform){
        const uint32_t mask=bits==32u?UINT32_MAX:((UINT32_C(1)<<bits)-1u);
        const uint32_t shifted=mask<<pos;
        const uint32_t existing=port_relocdrv_read32(dst);
        port_relocdrv_write(dst,4u,
            (existing&~shifted)|((value&mask)<<pos));
    }
    return PORT_RELOCDRV_OK;
}
