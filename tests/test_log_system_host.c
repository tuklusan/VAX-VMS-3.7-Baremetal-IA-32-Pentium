#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "vms_log_system.h"
#define CHECK(x) do {if(!(x)){fprintf(stderr,"LOGNAMSUB failed line %d: %s\n",__LINE__,#x);return 1;}}while(0)
struct fixture {
    uint32_t storage[PORT_V37_DEVICE_BYTES/4u];
    struct port_v37_device_result device;
    uint8_t disk[PORT_INI_LOG_TEMPLATE_BYTES];
    uint8_t sys[PORT_INI_LOG_TEMPLATE_BYTES];
    uint8_t sysnames[3][PORT_LOG_MAX_REC_LEN];
    struct port_ini_fin_result fin;
    struct port_log_system_table table;
};
static void make_log(uint8_t *r,const char *name,const char *value){
    const size_t nn=strlen(name),vn=strlen(value),len=23u+nn+vn;
    memset(r,0,PORT_LOG_MAX_REC_LEN);
    port_v37_write16(r+8u,(uint16_t)len);r[10u]=PORT_V37_DYN_LOG;
    r[11u]=PORT_LOG_SYSTEM;port_v37_write16(r+12u,1u);
    port_v37_write16(r+14u,1u);r[20u]=(uint8_t)nn;
    memcpy(r+21u,name,nn);r[21u+nn]=(uint8_t)vn;
    memcpy(r+22u+nn,value,vn);
}
static int prepare(struct fixture *f){
    memset(f,0,sizeof *f);
    const uint32_t base=(uint32_t)(uintptr_t)f->storage;
    CHECK(port_v37_devices_prelink((uint8_t *)f->storage,sizeof f->storage,
           base,&f->device)==PORT_V37_DEV_OK);
    const uint32_t null_ddt=0x00111000u;
    port_v37_write32((uint8_t *)f->storage+PORT_V37_DDB_NL_OFF+12u,null_ddt);
    port_v37_write32((uint8_t *)f->storage+PORT_V37_UCB_NL_OFF+120u,null_ddt);
    f->device.bound_nl_ddt=null_ddt;
    f->device.incomplete_driver_mask=7u;
    port_ini_log_templates(f->disk,f->sys);
    struct port_ini_fin_settings q={.device_bytes=(uint8_t *)f->storage,
        .device_size=sizeof f->storage,.device_base=base,
        .device_info=&f->device,.disk_log=f->disk,.sysdevice_log=f->sys,
        .disk_log_size=sizeof f->disk,.sysdevice_log_size=sizeof f->sys,
        .unit=0u,.concealed=1u,.adapter_is_ata=1u};
    CHECK(port_ini_fin_database(&q,&f->fin)==PORT_FINDB_OK);
    CHECK(port_ini_fin_check(&q,&f->fin)==PORT_FINDB_OK);
    return 0;
}
static int direct(void){
    CHECK(port_log_hash((const uint8_t *)"SYS$DISK",8u,4u)==12u);
    CHECK(port_log_hash((const uint8_t *)"SYS$SYSDEVICE",13u,4u)==13u);
    CHECK(port_log_hash((const uint8_t *)"SYS$DISK",8u,0u)==0u);
    CHECK(port_log_hash((const uint8_t *)"SYS$DISK",8u,2u)==3u);
    CHECK(port_log_hash((const uint8_t *)"SYS$SYSDEVICE",13u,2u)==3u);
    CHECK(port_log_hash((const uint8_t *)"SYS$DISK",8u,5u)<32u);
    CHECK(port_log_hash((const uint8_t *)"SYS$DISK",8u,14u)<16384u);
    CHECK(port_log_hash((const uint8_t *)"SYS$DISK",8u,15u)==UINT32_MAX);
    CHECK(port_log_hash((const uint8_t *)"SYS$DISK",0u,4u)==UINT32_MAX);
    CHECK(port_log_name_compare((const uint8_t *)"S",1u,(const uint8_t *)"SYS",3u)==0u);
    CHECK(port_log_name_compare((const uint8_t *)"SYS",3u,(const uint8_t *)"S",1u)==2u);
    CHECK(port_log_name_compare((const uint8_t *)"SYS",3u,(const uint8_t *)"SYS",3u)==1u);
    struct fixture f;
    CHECK(!prepare(&f));
    CHECK(port_log_system_validate(&f.table,f.disk,f.sys,sizeof f.disk)==PORT_LOG_OK);
    CHECK(port_log_system_init(&f.table,f.disk,f.sys,sizeof f.disk,4u)==PORT_LOG_OK);
    make_log(f.sysnames[0],"SYS$SYSTEM","DQA0:[TOPSYS.SYSEXE]");
    make_log(f.sysnames[1],"SYS$SHARE","DQA0:[TOPSYS.SYSLIB]");
    make_log(f.sysnames[2],"SYS$MESSAGE","DQA0:[TOPSYS.SYSMSG]");
    for(unsigned i=0;i<3u;i++)
        CHECK(port_log_system_add(&f.table,f.sysnames[i],
          (uint32_t)(uintptr_t)f.sysnames[i],sizeof f.sysnames[i])==PORT_LOG_OK);
    CHECK(f.table.count==5u&&port_log_system_check(&f.table)==PORT_LOG_OK);
    uint32_t system_address=0u;
    CHECK(port_log_system_lookup(&f.table,(const uint8_t *)"SYS$MESSAGE",11u,
          &system_address)==PORT_LOG_OK);
    CHECK(system_address==(uint32_t)(uintptr_t)f.sysnames[2]);
    uint8_t created_value[PORT_LOG_MAX_REC_LEN]={0};
    struct port_log_result result={0};
    CHECK(port_log_translate(&f.table,"SYS$SYSTEM",10u,0u,created_value,
          sizeof created_value,&result)==PORT_LOG_OK);
    CHECK(result.bytes==20u&&!memcmp(created_value,
          "DQA0:[TOPSYS.SYSEXE]",20u));
    CHECK(port_log_system_add(&f.table,f.sysnames[0],
          (uint32_t)(uintptr_t)f.sysnames[0],sizeof f.sysnames[0])==PORT_LOG_BAD);
    uint8_t duplicate[PORT_LOG_MAX_REC_LEN];
    make_log(duplicate,"SYS$SYSTEM","DQA0:[OTHER.SYSEXE]");
    CHECK(port_log_system_add(&f.table,duplicate,
          (uint32_t)(uintptr_t)duplicate,sizeof duplicate)==PORT_LOG_BAD);
    CHECK(f.table.count==5u&&port_log_system_check(&f.table)==PORT_LOG_OK);
    CHECK(port_log_system_check(&f.table)==PORT_LOG_OK);
    CHECK(f.table.head[12u]==(uint32_t)(uintptr_t)f.disk);
    CHECK(f.table.head[13u]==(uint32_t)(uintptr_t)f.sys);
    CHECK(port_v37_read32(f.disk)==0u && port_v37_read32(f.sys)==0u);
    CHECK(port_v37_read32(f.disk+4u)==(uint32_t)(uintptr_t)&f.table.head[12u]);
    CHECK(port_v37_read32(f.sys+4u)==(uint32_t)(uintptr_t)&f.table.head[13u]);
    uint32_t address=0xabcdefu;
    CHECK(port_log_system_lookup(&f.table,(const uint8_t *)"SYS$DISK",8u,&address)==PORT_LOG_OK);
    CHECK(address==(uint32_t)(uintptr_t)f.disk);
    CHECK(port_log_system_lookup(&f.table,(const uint8_t *)"SYS$MISSING",11u,&address)==PORT_LOG_NOTRAN);
    CHECK(address==(uint32_t)(uintptr_t)f.disk);
    uint8_t output[32]={0};
    CHECK(port_log_translate(&f.table,"SYS$DISK",8u,0u,output,sizeof output,&result)==PORT_LOG_OK);
    CHECK(result.bytes==6u&&result.table==0u&&result.record_address==(uint32_t)(uintptr_t)f.disk);
    CHECK(!memcmp(output,"_DQA0:",6u));
    memset(output,0,sizeof output);
    CHECK(port_log_translate(&f.table,"SYS$SYSDEVICE",13u,0u,output,sizeof output,&result)==PORT_LOG_OK);
    CHECK(result.bytes==7u&&!memcmp(output,"__DQA0:",7u));
    CHECK(port_log_translate(&f.table,"SYS$DISK",8u,1u,output,sizeof output,&result)==PORT_LOG_NOTRAN);
    CHECK(result.bytes==8u&&!memcmp(output,"SYS$DISK",8u));
    CHECK(port_log_translate(&f.table,"_DQA0:",6u,0u,output,sizeof output,&result)==PORT_LOG_NOTRAN);
    CHECK(result.bytes==5u&&!memcmp(output,"DQA0:",5u));
    memset(output,0xa5,sizeof output);
    CHECK(port_log_translate(&f.table,"SYS$DISK",8u,0u,output,3u,&result)==PORT_LOG_OVERFLOW);
    CHECK(result.bytes==6u&&output[0]==0xa5&&output[3]==0xa5);
    CHECK(port_log_translate(&f.table,"",0u,0u,output,sizeof output,&result)==PORT_LOG_BAD);
    CHECK(port_log_translate(&f.table,"SYS$DISK",8u,0u,output,sizeof output,0)==PORT_LOG_BAD);
    port_v37_write32(f.disk,(uint32_t)(uintptr_t)f.disk); /* malicious cycle */
    CHECK(port_log_system_check(&f.table)==PORT_LOG_CYCLE);
    CHECK(port_log_translate(&f.table,"SYS$DISK",8u,0u,output,sizeof output,&result)==PORT_LOG_BAD);
    port_v37_write32(f.disk,0u);
    f.disk[20u]=47u; /* invalid ASCIC offset, must not read beyond 48 B */
    CHECK(port_log_system_check(&f.table)==PORT_LOG_CYCLE);
    CHECK(port_log_translate(&f.table,"SYS$DISK",8u,0u,output,sizeof output,&result)==PORT_LOG_BAD);
    return 0;
}
static int collision(void){
    struct fixture f;CHECK(!prepare(&f));
    CHECK(port_log_system_init(&f.table,f.disk,f.sys,sizeof f.disk,0u)==PORT_LOG_OK);
    CHECK(port_log_system_check(&f.table)==PORT_LOG_OK);
    CHECK(f.table.head[0u]==(uint32_t)(uintptr_t)f.disk);
    CHECK(port_v37_read32(f.disk)==(uint32_t)(uintptr_t)f.sys);
    CHECK(port_v37_read32(f.sys+4u)==(uint32_t)(uintptr_t)f.disk);
    uint8_t out[9]={0};struct port_log_result r={0};
    CHECK(port_log_translate(&f.table,"SYS$SYSDEVICE",13u,0u,out,sizeof out,&r)==PORT_LOG_OK);
    CHECK(!memcmp(out,"__DQA0:",7u));
    CHECK(port_log_translate(&f.table,"SYS$DISK",8u,0u,out,sizeof out,&r)==PORT_LOG_OK);
    CHECK(!memcmp(out,"_DQA0:",6u));
    return 0;
}
static int rejection(void){
    struct fixture f;CHECK(!prepare(&f));
    f.sys[10u]=0u;
    CHECK(port_log_system_init(&f.table,f.disk,f.sys,sizeof f.disk,4u)==PORT_LOG_BAD);
    CHECK(!f.table.ready&&!f.table.count&&!port_v37_read32(f.disk));
    CHECK(!prepare(&f));
    f.disk[29u]=27u;
    CHECK(port_log_system_init(&f.table,f.disk,f.sys,sizeof f.disk,4u)==PORT_LOG_BAD);
    CHECK(!f.table.ready&&!f.table.count);
    CHECK(!prepare(&f));
    CHECK(port_log_system_init(&f.table,f.disk,f.sys,sizeof f.disk,5u)==PORT_LOG_BAD);
    CHECK(!f.table.ready);
    return 0;
}
int main(void){
    CHECK(!direct());CHECK(!collision());CHECK(!rejection());
    puts("LOGNAMSUB.MAR source SYSTEM LOG$HASH/SEARCHLOG/TRANSLATE initial names and overflow guards PASS");
    return 0;
}
