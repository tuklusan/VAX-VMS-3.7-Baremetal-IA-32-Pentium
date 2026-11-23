#include "vms_ini_fin_database.h"
/* Check independently after staging, including exact counted lengths,
 * pointer graph, no accidental activation of the DQ DDT and both names. */
uint32_t port_ini_fin_check(const struct port_ini_fin_settings *q,
    const struct port_ini_fin_result *r){
    if(!q||!r||!q->device_info||!q->disk_log||!q->sysdevice_log||
       !q->device_bytes||q->device_info->boot_name_ready!=1u||
       q->device_info->incomplete_driver_mask!=7u||
       !r->records_staged||r->concealed!=q->concealed ||
       r->boot_unit!=q->unit||r->boot_ddb!=q->device_info->ddb[0]||
       r->boot_ucb!=q->device_info->ucb[0]||
       port_v37_devices_check(q->device_bytes,q->device_size,q->device_base,
           q->device_info)!=PORT_V37_DEV_OK ||
       port_v37_read32(q->device_bytes+12u)!=0u ||
       port_v37_read32(q->device_bytes+PORT_V37_UCB_BOOT_OFF+120u)!=0u)
        return PORT_FINDB_MISMATCH;
    char physical[12],system[13];uint8_t pn=0u,sn=0u;
    if(port_ini_fin_physical(q->unit,physical,&pn)!=PORT_FINDB_OK||
       port_ini_fin_system(q->unit,q->concealed,system,&sn)!=PORT_FINDB_OK||
       r->physical_count!=pn || r->system_count!=sn ||
       q->disk_log[PORT_FINDB_DISK_EQ_OFF]!=pn||
       q->sysdevice_log[PORT_FINDB_SYSDEV_EQ_OFF]!=sn)
        return PORT_FINDB_MISMATCH;
    for(unsigned i=0;i<pn;i++)
        if(q->disk_log[PORT_FINDB_DISK_START+i]!=(uint8_t)physical[i]||
           r->physical[i]!=(uint8_t)physical[i])return PORT_FINDB_MISMATCH;
    for(unsigned i=0;i<sn;i++)
        if(q->sysdevice_log[PORT_FINDB_SYSDEV_START+i]!=(uint8_t)system[i]||
           r->system[i]!=(uint8_t)system[i])return PORT_FINDB_MISMATCH;
    return PORT_FINDB_OK;
}
