#include "vms_ini_fin_database.h"
/* INIT.MAR FIX_DRV_NAME precedes FIN_DATABASE, recording DQA and
 * DQDRIVER in the source DDB counted fields. Source DQ DDT still zero;
 * the PC ATA port translates device geometry without simulating RB730. */
uint32_t port_ini_fin_ddb_name(uint8_t *data,size_t cap,uint32_t base,
    struct port_v37_device_result *r){
    if(!data||!r||cap<PORT_V37_DEVICE_BYTES||
       base!=(uint32_t)(uintptr_t)data || r->boot_name_ready||
       r->incomplete_driver_mask!=7u ||
       port_v37_devices_check(data,cap,base,r)!=PORT_V37_DEV_OK)
        return PORT_FINDB_BAD;
    uint8_t *d=data+PORT_V37_DDB_BOOT_OFF;
    d[20u]=3u;
    d[21u]='D';d[22u]='Q';d[23u]='A';
    static const char drv[]="DQDRIVER";
    d[36u]=8u;
    for(unsigned i=0;i<8u;i++)d[37u+i]=(uint8_t)drv[i];
    r->boot_name_ready=1u;
    return PORT_FINDB_OK;
}
