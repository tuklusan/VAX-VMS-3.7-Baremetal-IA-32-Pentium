#include "vms_ini_fin_database.h"
/* Transactional source FIN_DATABASE: calculate entire logical strings
 * before mutating the fixed DDB and nonpaged LOG source records. */
uint32_t port_ini_fin_database(struct port_ini_fin_settings *q,
    struct port_ini_fin_result *out){
    if(!out||port_ini_fin_validate(q)!=PORT_FINDB_OK)
        return PORT_FINDB_BAD;
    char p[12],s[13];uint8_t pn=0u,sn=0u;
    if(port_ini_fin_physical(q->unit,p,&pn)!=PORT_FINDB_OK||
       port_ini_fin_system(q->unit,q->concealed,s,&sn)!=PORT_FINDB_OK||
       pn>PORT_INI_LOG_TEMPLATE_BYTES-PORT_FINDB_DISK_START||
       sn>PORT_INI_LOG_TEMPLATE_BYTES-PORT_FINDB_SYSDEV_START)
        return PORT_FINDB_NO_ROOM;
    if(port_ini_fin_ddb_name(q->device_bytes,q->device_size,
           q->device_base,q->device_info)!=PORT_FINDB_OK)
        return PORT_FINDB_BAD;
    port_ini_fin_disk_record(q->disk_log,p,pn);
    port_ini_fin_sysdev_record(q->sysdevice_log,s,sn);
    struct port_ini_fin_result result={0};
    for(unsigned i=0;i<pn;i++)result.physical[i]=(uint8_t)p[i];
    for(unsigned i=0;i<sn;i++)result.system[i]=(uint8_t)s[i];
    result.physical_count=pn;result.system_count=sn;
    result.boot_unit=q->unit;
    result.boot_ddb=q->device_info->ddb[0];
    result.boot_ucb=q->device_info->ucb[0];
    result.concealed=q->concealed;result.records_staged=1u;
    *out=result;
    return PORT_FINDB_OK;
}
