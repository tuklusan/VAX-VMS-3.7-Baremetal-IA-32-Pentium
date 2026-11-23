#include "vms_relocdrv.h"
/* A staged original DPT must not make an unimplemented DQA0 available to
 * user $ASSIGN or IO$_ functions. DDT/CRB bindings remain strictly absent. */
uint32_t port_relocdrv_dq_guard(const uint8_t *db,size_t cap,
    const struct port_v37_device_result *info){
    if(!db||!info||cap<PORT_V37_DEVICE_BYTES||
       info->incomplete_driver_mask!=7u||info->boot_name_ready!=1u||
       db[PORT_V37_DDB_BOOT_OFF+12u]||
       db[PORT_V37_DDB_BOOT_OFF+13u]||
       db[PORT_V37_DDB_BOOT_OFF+14u]||
       db[PORT_V37_DDB_BOOT_OFF+15u]||
       db[PORT_V37_UCB_BOOT_OFF+120u]||
       db[PORT_V37_UCB_BOOT_OFF+121u]||
       db[PORT_V37_UCB_BOOT_OFF+122u]||
       db[PORT_V37_UCB_BOOT_OFF+123u])return PORT_RELOCDRV_UNBOUND;
    return PORT_RELOCDRV_OK;
}
