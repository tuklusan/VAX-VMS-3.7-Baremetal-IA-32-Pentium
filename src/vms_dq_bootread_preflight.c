#include "vms_dq_bootread.h"
/* Privileged-only DQ physical block path. Early INIT can run before DDT
 * publication; active STARTIO supplies the validated published DDT base. */
static int disjoint(const void *a,size_t as,const void *b,size_t bs){
    uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
    if(as>UINTPTR_MAX-x||bs>UINTPTR_MAX-y)return 0;
    return x+as<=y||y+bs<=x;
}
uint32_t port_dq_bootread_preflight(const struct port_dq_bootread_request *q){
    if(!q||!q->irp||!q->sector||!q->device_info||!q->device_db||
       !q->fdt||q->irp_bytes!=PORT_V37_IRP_SIZE||
       q->sector_bytes!=PORT_DQ_BOOTREAD_BYTES||q->boot_unit!=0u||
       ((uintptr_t)q->irp&3u)||((uintptr_t)q->sector&1u)||
       !disjoint(q->sector,q->sector_bytes,q->irp,q->irp_bytes)||
       !disjoint(q->sector,q->sector_bytes,q->device_db,PORT_V37_DEVICE_BYTES)||
       !disjoint(q->irp,q->irp_bytes,q->device_db,PORT_V37_DEVICE_BYTES)||
       !disjoint(q->sector,q->sector_bytes,q->fdt,sizeof(*q->fdt))||
       !disjoint(q->irp,q->irp_bytes,q->fdt,sizeof(*q->fdt)))
        return PORT_DQ_BOOTREAD_BAD;
    const uint16_t function=port_v37_read16(q->irp+PORT_V37_IRP_FUNC);
    if((function==PORT_DQ_READPBLK&&!q->read_sector)||
       (function==PORT_DQ_WRITEPBLK&&!q->write_sector)||
       (function!=PORT_DQ_READPBLK&&function!=PORT_DQ_WRITEPBLK))
        return PORT_DQ_BOOTREAD_BAD;
    const uint32_t mask=q->device_info->incomplete_driver_mask;
    if(port_v37_devices_check(q->device_db,PORT_V37_DEVICE_BYTES,
         (uint32_t)(uintptr_t)q->device_db,q->device_info)!=PORT_V37_DEV_OK ||
       !q->device_info->bound_nl_ddt ||
       port_dq_fdt_validate(q->fdt,sizeof(*q->fdt))!=PORT_DQ_FDT_OK)
        return PORT_DQ_BOOTREAD_BAD;
    const uint8_t *u=q->device_db+PORT_V37_UCB_BOOT_OFF;
    const int prepublish=mask==7u&&q->active_ddt_base==0u;
    const int published=mask==6u&&q->active_ddt_base!=0u&&
       q->active_ddt_base==q->device_info->bound_dqa_ddt&&
       q->device_info->bound_dqa_crb!=0u&&
       port_v37_read32(u+32u)==q->device_info->bound_dqa_crb;
    if(!prepublish&&!published)return PORT_DQ_BOOTREAD_BAD;
    if(q->device_info->ucb[0]!=(uint32_t)(uintptr_t)u ||
       port_v37_read16(u+8u)!=PORT_V37_BOOT_UCB_SIZE ||
       port_v37_read32(u+36u)!=q->device_info->ddb[0] ||
       port_v37_read16(u+58u)!=512u ||
       port_v37_read16(u+72u)!=q->boot_unit ||
       port_v37_read32(u+120u)!=q->active_ddt_base ||
       port_v37_read32(q->device_db+PORT_V37_DDB_BOOT_OFF+12u)!=
           q->active_ddt_base ||
       port_v37_read32(q->irp+PORT_V37_IRP_UCB)!=q->device_info->ucb[0] ||
       port_v37_read16(q->irp+8u)!=PORT_V37_IRP_SIZE ||
       q->irp[10u]!=PORT_V37_DYN_IRP ||
       q->irp[11u]!=0u ||  /* kernel mode */
       port_v37_read16(q->irp+42u)!=PORT_DQ_PHYSIO ||
       port_v37_read32(q->irp+36u)!=0u || /* No process IOSB */
       port_v37_read16(q->irp+40u)!=0u || /* No user channel */
       port_v37_read32(q->irp+44u)!=0u || /* No user PTEs */
       port_v37_read16(q->irp+50u)!=PORT_DQ_BOOTREAD_BYTES ||
       port_v37_read16(q->irp+48u)!=0u ||
       port_v37_read32(q->irp+PORT_V37_IRP_IOST2)!=0u)
        return PORT_DQ_BOOTREAD_BAD;
    return PORT_DQ_BOOTREAD_OK;
}
