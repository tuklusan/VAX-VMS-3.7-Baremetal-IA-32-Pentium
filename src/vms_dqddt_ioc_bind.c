#include "vms_dqddt_ioc_bind.h"
#include "vms_ini_devicedb.h"

static int overlaps32(uint32_t a,uint32_t an,uint32_t b,uint32_t bn){
    return (uint64_t)a<(uint64_t)b+bn&&(uint64_t)b<(uint64_t)a+an;
}

/* Publish only the validated native DDT/FDT and the already attached CRB.
 * DQA remains a partial driver: STARTIO has physical READPBLK/WRITEPBLK
 * actions and all unsupported actions return unbound. */
uint32_t port_dqddt_publish_ioc(uint8_t *db,size_t capacity,uint32_t db_base,
    struct port_v37_device_result *info,const struct port_dqddt_stage *stage,
    const struct port_dq_fdt *fdt,const struct port_dqddt_native *native,
    const struct port_dq_crb *crb){
    if(!db||!info||!stage||!fdt||!native||!crb||
       capacity<PORT_V37_DEVICE_BYTES||
       db_base!=(uint32_t)(uintptr_t)db||
       info->incomplete_driver_mask!=7u||info->boot_name_ready!=1u||
       !info->bound_nl_ddt||info->bound_dqa_ddt||!info->bound_dqa_crb||
       info->bound_dqa_crb!=(uint32_t)(uintptr_t)crb||
       native->base!=(uint32_t)(uintptr_t)native->image||
       overlaps32(db_base,PORT_V37_DEVICE_BYTES,native->base,
                  PORT_DQDDT_IMAGE_BYTES)||
       overlaps32(db_base,PORT_V37_DEVICE_BYTES,
                  (uint32_t)(uintptr_t)crb,sizeof(*crb))||
       port_v37_devices_check(db,capacity,db_base,info)!=PORT_V37_DEV_OK||
       port_dq_crb_validate(crb)!=PORT_DQ_CRB_OK||
       port_v37_read32(db+PORT_V37_UCB_BOOT_OFF+32u)!=info->bound_dqa_crb||
       port_dqddt_native_validate(stage,fdt,native)!=PORT_DQDDT_OK)
        return PORT_DQDDT_BIND_BAD;
    const struct port_v37_device_result prior=*info;
    struct port_v37_device_result candidate=prior;
    candidate.bound_dqa_ddt=native->base;
    candidate.incomplete_driver_mask&=~1u;
    port_v37_write32(db+PORT_V37_DDB_BOOT_OFF+12u,native->base);
    port_v37_write32(db+PORT_V37_UCB_BOOT_OFF+120u,native->base);
    *info=candidate;
    if(port_v37_devices_check(db,capacity,db_base,info)==PORT_V37_DEV_OK)
        return PORT_DQDDT_BIND_OK;
    port_v37_write32(db+PORT_V37_DDB_BOOT_OFF+12u,0u);
    port_v37_write32(db+PORT_V37_UCB_BOOT_OFF+120u,0u);
    *info=prior;
    return PORT_DQDDT_BIND_BAD;
}
