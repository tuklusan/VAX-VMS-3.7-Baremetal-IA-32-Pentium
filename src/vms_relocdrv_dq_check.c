#include "vms_relocdrv.h"
/* Source DQ DPT_STORE resulting bytes. Pointer fields remain unbound,
 * so a diagnostic observation is not a published device driver. */
uint32_t port_relocdrv_dq_check(const struct port_relocdrv_dq *d){
    if(!d||!d->ready||d->driver_bound||
       d->applied!=PORT_RELOCDRV_DQ_INIT_COUNT||
       !d->source_ddb||!d->source_ucb||
       d->ddb[16u]!='F'||d->ddb[17u]!='1'||d->ddb[18u]!='1'||
       d->ddb[19u]!=1u||d->ucb[11u]!=8u||
       port_relocdrv_read32(d->ucb+52u)!=PORT_RELOCDRV_DQ_CHAR||
       d->ucb[56u]!=1u||port_relocdrv_read16(d->ucb+58u)!=512u||
       d->ucb[82u]!=21u||d->ucb[113u]!=8u||
       port_relocdrv_read32(d->ucb+PORT_RELOCDRV_DQ_CURDA)!=UINT32_MAX||
       port_relocdrv_read32(d->ddb+12u)!=0u||
       port_relocdrv_read32(d->ucb+120u)!=0u||
       port_relocdrv_read32(d->ucb+32u)!=0u)
        return PORT_RELOCDRV_BAD;
    return PORT_RELOCDRV_OK;
}
