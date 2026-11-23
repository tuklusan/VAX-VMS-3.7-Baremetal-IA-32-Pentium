#include "vms_relocdrv.h"
/* Original IOC$INITDRV reads DPT$W_INITTAB and loops LOCATE/EXECUTE until
 * the zero structure terminator. Enforce an explicit instruction cap. */
uint32_t port_relocdrv_walk(const struct port_relocdrv_context *c,
    const uint8_t *table,size_t len,int perform,uint32_t *done){
    if(!c||!table||!len||!done)return PORT_RELOCDRV_BAD;
    size_t at=0;uint32_t count=0;
    while(at<len){
        if(!table[at]){
            if(at+1u!=len)return PORT_RELOCDRV_BAD;
            *done=count;return PORT_RELOCDRV_OK;
        }
        if(count>=PORT_RELOCDRV_OP_CAP)return PORT_RELOCDRV_OVERFLOW;
        struct port_relocdrv_op op={0};
        if(port_relocdrv_decode(table,len,at,&op)!=PORT_RELOCDRV_OK)
            return PORT_RELOCDRV_BAD;
        const uint32_t r=port_relocdrv_step(c,&op,perform);
        if(r!=PORT_RELOCDRV_OK)return r;
        at=op.next;count++;
    }
    return PORT_RELOCDRV_BAD;
}
