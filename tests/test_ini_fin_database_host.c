#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "vms_ini_fin_database.h"
#define CHECK(x) do{if(!(x)){fprintf(stderr,"FIN_DATABASE fail line %d: %s\n",__LINE__,#x);return 1;}}while(0)
static int stage(uint8_t conceal){
    uint32_t storage[PORT_V37_DEVICE_BYTES/4u]={0};
    uint8_t disk[PORT_INI_LOG_TEMPLATE_BYTES],sys[PORT_INI_LOG_TEMPLATE_BYTES];
    struct port_v37_device_result db={0};
    uint32_t address=(uint32_t)(uintptr_t)storage;
    CHECK(address&&!(address&3u));
    CHECK(port_v37_devices_prelink((uint8_t *)storage,sizeof storage,address,&db)==PORT_V37_DEV_OK);
    const uint32_t nlddt=0x00111000u;
    port_v37_write32((uint8_t*)storage+PORT_V37_DDB_NL_OFF+12u,nlddt);
    port_v37_write32((uint8_t*)storage+PORT_V37_UCB_NL_OFF+120u,nlddt);
    db.incomplete_driver_mask=7u;db.bound_nl_ddt=nlddt;
    port_ini_log_templates(disk,sys);
    struct port_ini_fin_settings req={.device_bytes=(uint8_t *)storage,
        .device_size=sizeof storage,.device_base=address,.device_info=&db,
        .disk_log=disk,.sysdevice_log=sys,.disk_log_size=sizeof disk,
        .sysdevice_log_size=sizeof sys,.unit=0u,
        .concealed=conceal,.adapter_is_ata=1u};
    struct port_ini_fin_result out={.physical_count=0xf0u};
    CHECK(port_ini_fin_validate(&req)==PORT_FINDB_OK);
    CHECK(port_ini_fin_database(&req,&out)==PORT_FINDB_OK);
    CHECK(port_ini_fin_check(&req,&out)==PORT_FINDB_OK);
    CHECK(db.boot_name_ready==1u&&db.incomplete_driver_mask==7u);
    CHECK(port_v37_read32((uint8_t *)storage+12u)==0u); /* disk DDT unbound */
    CHECK(((uint8_t *)storage)[20u]==3u && !memcmp((uint8_t*)storage+21u,"DQA",3u));
    CHECK(((uint8_t *)storage)[36u]==8u&&!memcmp((uint8_t*)storage+37u,"DQDRIVER",8u));
    CHECK(disk[29u]==6u&&!memcmp(disk+30u,"_DQA0:",6u));
    if(conceal){CHECK(sys[34u]==7u&&!memcmp(sys+35u,"__DQA0:",7u));}
    else {CHECK(sys[34u]==6u&&!memcmp(sys+35u,"_DQA0:",6u));}
    CHECK(out.physical_count==6u&&out.system_count==(conceal?7u:6u));
    CHECK(out.boot_unit==0u&&out.boot_ddb==address&&out.boot_ucb==db.ucb[0]);
    CHECK(port_ini_fin_database(&req,&out)==PORT_FINDB_BAD);
    CHECK(port_ini_fin_check(&req,&out)==PORT_FINDB_OK);
    disk[30u]='X';
    CHECK(port_ini_fin_check(&req,&out)==PORT_FINDB_MISMATCH);
    disk[30u]='_';
    db.boot_name_ready=0u;
    CHECK(port_ini_fin_check(&req,&out)==PORT_FINDB_MISMATCH);
    db.boot_name_ready=1u;
    CHECK(port_ini_fin_check(&req,&out)==PORT_FINDB_OK);
    return 0;
}
static int failure_paths(void){
    uint32_t storage[PORT_V37_DEVICE_BYTES/4u]={0};
    uint8_t disk[PORT_INI_LOG_TEMPLATE_BYTES],sys[PORT_INI_LOG_TEMPLATE_BYTES];
    struct port_v37_device_result db={0};
    const uint32_t base=(uint32_t)(uintptr_t)storage;
    CHECK(port_v37_devices_prelink((uint8_t *)storage,sizeof storage,base,&db)==PORT_V37_DEV_OK);
    port_v37_write32((uint8_t*)storage+PORT_V37_DDB_NL_OFF+12u,0x00700000u);
    port_v37_write32((uint8_t*)storage+PORT_V37_UCB_NL_OFF+120u,0x00700000u);
    db.incomplete_driver_mask=7u;db.bound_nl_ddt=0x00700000u;
    port_ini_log_templates(disk,sys);
    struct port_ini_fin_settings req={.device_bytes=(uint8_t*)storage,
      .device_size=sizeof storage,.device_base=base,.device_info=&db,
      .disk_log=disk,.sysdevice_log=sys,.disk_log_size=sizeof disk,
      .sysdevice_log_size=sizeof sys,.unit=0u,.concealed=1u,.adapter_is_ata=1u};
    uint8_t olddb[PORT_V37_DEVICE_BYTES],olddisk[48],oldsys[48];
    memcpy(olddb,storage,sizeof olddb);
    memcpy(olddisk,disk,sizeof olddisk);
    memcpy(oldsys,sys,sizeof oldsys);
    struct port_ini_fin_result result={.physical_count=0xab};
    req.adapter_is_ata=0u;
    CHECK(port_ini_fin_database(&req,&result)==PORT_FINDB_BAD);
    req.adapter_is_ata=1u;req.unit=1u;
    CHECK(port_ini_fin_database(&req,&result)==PORT_FINDB_BAD);
    req.unit=0u;req.concealed=2u;
    CHECK(port_ini_fin_database(&req,&result)==PORT_FINDB_BAD);
    req.concealed=1u;req.disk_log_size=40u;
    CHECK(port_ini_fin_database(&req,&result)==PORT_FINDB_BAD);
    req.disk_log_size=48u;sys[20u]='*';
    CHECK(port_ini_fin_database(&req,&result)==PORT_FINDB_BAD);
    sys[20u]=13u;
    CHECK(!memcmp(storage,olddb,sizeof olddb));
    CHECK(!memcmp(disk,olddisk,sizeof olddisk));
    CHECK(!memcmp(sys,oldsys,sizeof oldsys));
    CHECK(result.physical_count==0xab);
    CHECK(port_ini_fin_database(0,&result)==PORT_FINDB_BAD);
    CHECK(port_ini_fin_database(&req,0)==PORT_FINDB_BAD);
    return 0;
}
int main(void){
    const uint16_t units[]={0u,9u,10u,99u,100u,999u,1000u,9999u,65535u};
    const char *const digits[]={"0","9","10","99","100","999","1000","9999","65535"};
    for(unsigned i=0;i<sizeof units/sizeof units[0];i++){
        char physical[12],system[13],decimal[5]={0};uint8_t n=0u,sysn=0u;
        CHECK(port_ini_fin_decimal(units[i],decimal)==strlen(digits[i]));
        CHECK(!memcmp(decimal,digits[i],strlen(digits[i])));
        CHECK(port_ini_fin_physical(units[i],physical,&n)==PORT_FINDB_OK);
        CHECK(n==5u+strlen(digits[i]));
        CHECK(!memcmp(physical,"_DQA",4u));
        CHECK(!memcmp(physical+4u,digits[i],strlen(digits[i])));
        CHECK(physical[n-1u]==':');
        CHECK(port_ini_fin_system(units[i],1u,system,&sysn)==PORT_FINDB_OK);
        CHECK(sysn==n+1u);
        CHECK(system[0u]=='_'&&!memcmp(system+1u,physical,n));
    }
    CHECK(stage(0u)==0);
    CHECK(stage(1u)==0);
    CHECK(failure_paths()==0);
    puts("INIT.MAR FIN_DATABASE source DQA boot name and counted SYS$DISK/SYS$SYSDEVICE logical records, bounds and unit conversion PASS");
    return 0;
}
