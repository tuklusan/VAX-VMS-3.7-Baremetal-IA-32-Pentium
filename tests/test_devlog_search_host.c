#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "vms_devlog_search.h"
#define CHECK(x) do {if(!(x)){fprintf(stderr,"IOC LOG failed line %d: %s\n",__LINE__,#x);return 1;}}while(0)
struct fixture {
    uint32_t storage[PORT_V37_DEVICE_BYTES/4u];
    struct port_v37_device_result device;
    uint8_t disk[PORT_INI_LOG_TEMPLATE_BYTES];
    uint8_t sysdev[PORT_INI_LOG_TEMPLATE_BYTES];
    struct port_ini_fin_result fin;
    struct port_log_system_table table;
};
static int init(struct fixture *f){
    memset(f,0,sizeof *f);
    const uint32_t base=(uint32_t)(uintptr_t)f->storage;
    CHECK(port_v37_devices_prelink((uint8_t *)f->storage,sizeof f->storage,
          base,&f->device)==PORT_V37_DEV_OK);
    const uint32_t null_ddt=0x00111000u;
    port_v37_write32((uint8_t *)f->storage+PORT_V37_DDB_NL_OFF+12u,null_ddt);
    port_v37_write32((uint8_t *)f->storage+PORT_V37_UCB_NL_OFF+120u,null_ddt);
    f->device.bound_nl_ddt=null_ddt;
    f->device.incomplete_driver_mask=7u;
    port_ini_log_templates(f->disk,f->sysdev);
    struct port_ini_fin_settings settings={
       .device_bytes=(uint8_t *)f->storage,.device_size=sizeof f->storage,
       .device_base=base,.device_info=&f->device,.disk_log=f->disk,
       .sysdevice_log=f->sysdev,.disk_log_size=sizeof f->disk,
       .sysdevice_log_size=sizeof f->sysdev,.unit=0u,.concealed=1u,
       .adapter_is_ata=1u};
    CHECK(port_ini_fin_database(&settings,&f->fin)==PORT_FINDB_OK);
    CHECK(port_log_system_init(&f->table,f->disk,f->sysdev,sizeof f->disk,
                              PORT_LOG_BITS)==PORT_LOG_OK);
    return 0;
}
static uint32_t find(struct fixture *f,const char *name,size_t len,
                     struct port_devlog_trace *t){
    return port_devlog_search(&f->table,(uint8_t *)f->storage,
        sizeof f->storage,(uint32_t)(uintptr_t)f->storage,
        &f->device,name,len,PORT_V37_IOC_MODE_PHYSICAL,t);
}
int main(void){
    struct fixture f;
    CHECK(!init(&f));
    const uint32_t original[3]={f.device.ucb[0],f.device.ucb[1],f.device.ucb[4]};
    struct port_devlog_trace t={0};
    /* Original INIT storage is visible, but DQA has no published UCB path. */
    CHECK(find(&f,"sys$disk",8u,&t)==PORT_V37_IOC_UNBOUND);
    CHECK(t.translations==1u&&t.length==6u&&!t.concealed&&
          !memcmp(t.physical,"_DQA0:",6u)&&!t.driver_bound&&t.device.ucb==f.device.ucb[0]);
    CHECK(find(&f,"SYS$DISK:",9u,&t)==PORT_V37_IOC_UNBOUND);
    CHECK(t.translations==1u&&t.length==6u&&!memcmp(t.physical,"_DQA0:",6u));
    CHECK(find(&f,"SYS$SYSDEVICE",13u,&t)==PORT_V37_IOC_UNBOUND);
    CHECK(t.translations==1u&&t.length==7u&&t.concealed&&
          !memcmp(t.physical,"__DQA0:",7u)&&t.device.ucb==f.device.ucb[0]);
    CHECK(find(&f,"NLA0:",5u,&t)==PORT_V37_IOC_OK);
    CHECK(t.translations==1u&&t.last_notran&&t.length==4u&&
          !memcmp(t.physical,"NLA0",4u)&&t.driver_bound&&
          t.device.ucb==f.device.ucb[4]);
    CHECK(find(&f,"__nla0:",7u,&t)==PORT_V37_IOC_OK);
    CHECK(t.translations==0u&&t.concealed&&t.device.ucb==f.device.ucb[4]);
    CHECK(find(&f,"_OPA0:",6u,&t)==PORT_V37_IOC_UNBOUND);
    CHECK(t.translations==0u&&!t.concealed&&!t.driver_bound&&
          t.device.ucb==f.device.ucb[1]);
    CHECK(find(&f,"MBA1:",5u,&t)==PORT_V37_IOC_UNBOUND);
    CHECK(t.device.ucb==f.device.ucb[2]&&t.translations==1u);
    /* Source R0 failure must never return a fake UCB or alter caller output. */
    const struct port_devlog_trace before=t;
    CHECK(find(&f,"N::NLA0",7u,&t)==PORT_V37_IOC_NONLOCAL);
    CHECK(!memcmp(&before,&t,sizeof t));
    CHECK(find(&f,"SYS$UNKNOWN",11u,&t)==PORT_V37_IOC_LOGICAL_PENDING);
    CHECK(!memcmp(&before,&t,sizeof t));
    CHECK(find(&f,"___NLA0",7u,&t)==PORT_V37_IOC_BAD);
    CHECK(!memcmp(&before,&t,sizeof t));
    CHECK(find(&f,"OPA65536",8u,&t)==PORT_V37_IOC_BAD);
    CHECK(!memcmp(&before,&t,sizeof t));
    CHECK(find(&f,"NLA0:FILE",9u,&t)==PORT_V37_IOC_BAD);
    CHECK(!memcmp(&before,&t,sizeof t));
    CHECK(find(&f,"NLA0:\0E",7u,&t)==PORT_V37_IOC_BAD);
    CHECK(!memcmp(&before,&t,sizeof t));
    CHECK(find(&f,"",0u,&t)==PORT_V37_IOC_BAD);
    CHECK(!memcmp(&before,&t,sizeof t));
    CHECK(port_devlog_search(&f.table,(uint8_t *)f.storage,sizeof f.storage,
        (uint32_t)(uintptr_t)f.storage,&f.device,"NLA0",4u,0u,&t)==PORT_V37_IOC_BAD);
    CHECK(!memcmp(&before,&t,sizeof t));
    CHECK(f.device.ucb[0]==original[0]&&f.device.ucb[1]==original[1]&&
          f.device.ucb[4]==original[2]);
    CHECK(port_v37_read32((uint8_t *)f.storage+12u)==0u);
    CHECK(!init(&f));
    port_v37_write32(f.disk,(uint32_t)(uintptr_t)f.disk);
    CHECK(find(&f,"SYS$DISK",8u,&t)==PORT_V37_IOC_BAD);
    CHECK(!memcmp(&before,&t,sizeof t));
    puts("IOSUBPAGD.MAR source logical->physical IOC lookup: SYSTEM aliases, no DQ bind, remote/invalid guards PASS");
    return 0;
}
