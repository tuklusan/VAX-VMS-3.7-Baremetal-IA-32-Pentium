#include "vms_ini_fin_database.h"
/* Preflight all DDB and LOG record bounds before changing resident data.
 * A trusted IA-32 boot adapter must already have read the real ODS-2 home.
 * Presence of a DQ source FDT alone is insufficient. */
static int disjoint(const uint8_t *a,size_t n,const uint8_t *b,size_t k){
    const uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
    return n<=UINTPTR_MAX-x&&k<=UINTPTR_MAX-y&&(x+n<=y||y+k<=x);
}
static int record(const uint8_t *d,size_t len,const char *name,uint8_t nchars,
    uint32_t expected_count){
    if(!d||len<PORT_INI_LOG_TEMPLATE_BYTES||
       port_v37_read16(d+8u)!=PORT_INI_LOG_TEMPLATE_BYTES||
       d[10u]!=PORT_V37_DYN_LOG||d[11u]!=0u||
       port_v37_read16(d+12u)!=1u||port_v37_read16(d+14u)!=1u||
       port_v37_read32(d+16u)!=0u||d[20u]!=nchars ||
       d[20u+nchars+1u]!=(uint8_t)expected_count)return 0;
    for(unsigned i=0;i<nchars;i++)if(d[21u+i]!=(uint8_t)name[i])return 0;
    return 1;
}
uint32_t port_ini_fin_validate(const struct port_ini_fin_settings *q){
    if(!q||!q->device_bytes||!q->device_info||!q->disk_log||
       !q->sysdevice_log||q->concealed>1u||q->adapter_is_ata!=1u||
       q->unit!=0u||q->device_size<PORT_V37_DEVICE_BYTES||
       q->disk_log_size<PORT_INI_LOG_TEMPLATE_BYTES||
       q->sysdevice_log_size<PORT_INI_LOG_TEMPLATE_BYTES||
       q->device_base!=(uint32_t)(uintptr_t)q->device_bytes||
       !disjoint(q->device_bytes,PORT_V37_DEVICE_BYTES,q->disk_log,PORT_INI_LOG_TEMPLATE_BYTES)||
       !disjoint(q->device_bytes,PORT_V37_DEVICE_BYTES,q->sysdevice_log,PORT_INI_LOG_TEMPLATE_BYTES)||
       !disjoint(q->disk_log,PORT_INI_LOG_TEMPLATE_BYTES,q->sysdevice_log,PORT_INI_LOG_TEMPLATE_BYTES) ||
       port_v37_devices_check(q->device_bytes,q->device_size,q->device_base,
             q->device_info)!=PORT_V37_DEV_OK ||
       q->device_info->incomplete_driver_mask!=7u||
       q->device_info->boot_name_ready!=0u ||
       port_v37_read16(q->device_bytes+PORT_V37_UCB_BOOT_OFF+72u)!=q->unit ||
       !record(q->disk_log,q->disk_log_size,"SYS$DISK",8u,5u) ||
       !record(q->sysdevice_log,q->sysdevice_log_size,"SYS$SYSDEVICE",13u,1u))
        return PORT_FINDB_BAD;
    return PORT_FINDB_OK;
}
