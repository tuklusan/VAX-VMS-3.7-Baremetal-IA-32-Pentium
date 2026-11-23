#include "vms_relocdrv.h"
/* RELOCDRV INITDRV on native IA-32: a full validation pass prevents any
 * partial mutation of DDB/UCB when an otherwise valid table later fails. */
uint32_t port_relocdrv_apply(const struct port_relocdrv_context *c,
    const uint8_t *table,size_t len,uint32_t *done){
    if(!done||port_relocdrv_isolated(c,table,len)!=PORT_RELOCDRV_OK)
        return PORT_RELOCDRV_BAD;
    uint32_t expected=0,applied=0;
    const uint32_t valid=port_relocdrv_walk(c,table,len,0,&expected);
    if(valid!=PORT_RELOCDRV_OK)return valid;
    const uint32_t result=port_relocdrv_walk(c,table,len,1,&applied);
    if(result!=PORT_RELOCDRV_OK||applied!=expected)
        return PORT_RELOCDRV_BAD;
    *done=applied;return PORT_RELOCDRV_OK;
}
