#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "vms_dq_bootread.h"
#define CHECK(c) do{if(!(c)){fprintf(stderr,"DQ boot READPBLK failed line %d: %s\n",__LINE__,#c);return 1;}}while(0)
static unsigned reads,last_lbn;
static int read_home(void *ctx,uint32_t lbn,uint16_t dst[256]){
    uint16_t *const block=ctx;
    ++reads;last_lbn=lbn;
    for(unsigned i=0;i<256u;i++)dst[i]=block[i];
    return 0;
}
static int disk_error(void *ctx,uint32_t lbn,uint16_t dst[256]){
    (void)ctx;(void)lbn;
    ++reads;
    for(unsigned i=0;i<256u;i++)dst[i]=0xdeadu;
    return 1;
}
static int fails_when(int (*mutate)(struct port_dq_bootread_request *),
    struct port_dq_bootread_request *q){
    uint8_t old_irp[PORT_V37_IRP_SIZE];
    uint8_t old_db[PORT_V37_DEVICE_BYTES];
    uint16_t old_sector[256];
    struct port_dq_bootread_result result={.irp_status=0xbeef};
    if(mutate && mutate(q))return 0;
    memcpy(old_irp,q->irp,sizeof old_irp);
    memcpy(old_db,q->device_db,sizeof old_db);
    memcpy(old_sector,q->sector,sizeof old_sector);
    const uint32_t code=port_dq_bootread_execute(q,&result);
    if(code==PORT_DQ_BOOTREAD_OK || result.irp_status!=0xbeef ||
       memcmp(old_irp,q->irp,sizeof old_irp) ||
       memcmp(old_db,q->device_db,sizeof old_db) ||
       memcmp(old_sector,q->sector,sizeof old_sector))return 0;
    return 1;
}
static int bad_func(struct port_dq_bootread_request *q){
    port_v37_write16(q->irp+PORT_V37_IRP_FUNC,PORT_DQ_WRITELBLK);return 0;
}
static int bad_media(struct port_dq_bootread_request *q){
    port_v37_write32(q->irp+PORT_V37_IRP_MEDIA,2u);return 0;
}
static int bad_status(struct port_dq_bootread_request *q){
    port_v37_write16(q->irp+42u,0u);return 0;
}
int main(void){
    uint32_t devices[PORT_V37_DEVICE_BYTES/4u]={0};
    uint32_t irp_storage[PORT_V37_IRP_SIZE/4u]={0};
    uint16_t read_result[256]={0};
    uint16_t original_block[256]={0};
    struct port_v37_device_result info={0};
    struct port_dq_fdt fdt={0};
    const uint32_t addr=(uint32_t)(uintptr_t)devices;
    CHECK(addr && !(addr&3u));
    CHECK(port_v37_devices_prelink((uint8_t *)devices,sizeof devices,addr,&info)==PORT_V37_DEV_OK);
    const uint32_t nl_ddt=0x00700000u;
    /* NLA is the only DDT bound at this stage. BOOT DDT/CRB is absent. */
    port_v37_write32((uint8_t *)devices+PORT_V37_DDB_NL_OFF+12u,nl_ddt);
    port_v37_write32((uint8_t *)devices+PORT_V37_UCB_NL_OFF+120u,nl_ddt);
    info.bound_nl_ddt=nl_ddt;
    info.incomplete_driver_mask=7u;
    CHECK(port_v37_devices_check((const uint8_t *)devices,sizeof devices,addr,&info)==PORT_V37_DEV_OK);
    CHECK(port_dq_fdt_build(&fdt,sizeof fdt)==PORT_DQ_FDT_OK);
    CHECK(port_v37_irp_prepare((uint8_t *)irp_storage,PORT_V37_IRP_SIZE,
       0x00712000u,info.ucb[0],PORT_DQ_READPBLK)==PORT_V37_QIO_OK);
    uint8_t *const irp=(uint8_t *)irp_storage;
    port_v37_write16(irp+42u,PORT_DQ_PHYSIO);
    port_v37_write16(irp+50u,PORT_DQ_BOOTREAD_BYTES);
    port_v37_write32(irp+PORT_V37_IRP_MEDIA,PORT_DQ_BOOTREAD_LBN);
    FILE *file=fopen("portdisk.img","rb");
    CHECK(file!=0);
    CHECK(fseek(file,512L,SEEK_SET)==0);
    CHECK(fread(original_block,1,512u,file)==512u);
    CHECK(fclose(file)==0);
    CHECK(original_block[0]==1u); /* Genuine source Files-11 home LBN */
    struct port_dq_bootread_request q={
        .fdt=&fdt,.device_db=(uint8_t *)devices,.device_info=&info,
        .irp=irp,.irp_bytes=PORT_V37_IRP_SIZE,.sector=read_result,
        .sector_bytes=512u,.boot_unit=0u,.read_sector=read_home,
        .read_context=original_block};
    struct port_dq_bootread_result result={0};
    CHECK(port_dq_bootread_preflight(&q)==PORT_DQ_BOOTREAD_OK);
    CHECK(port_dq_bootread_execute(&q,&result)==PORT_DQ_BOOTREAD_OK);
    CHECK(reads==1u && last_lbn==1u);
    CHECK(!memcmp(read_result,original_block,sizeof read_result));
    CHECK(result.lbn==1u&&result.bytes==512u&&result.irp_status==1u&&
          result.irp_second==512u && result.ucb_io_count==1u &&
          result.ucb_media==1u && result.io_function==12u &&
          result.sector_checksum==port_dq_bootread_checksum(original_block));
    CHECK(port_v37_read32(irp+56u)==1u && port_v37_read32(irp+60u)==512u);
    CHECK(port_v37_read32((uint8_t *)devices+PORT_V37_UCB_BOOT_OFF+156u)==1u);
    CHECK(port_v37_read32((uint8_t *)devices+PORT_V37_UCB_BOOT_OFF+96u)==1u);
    CHECK(port_dq_bootread_execute(&q,&result)==PORT_DQ_BOOTREAD_BAD);
    CHECK(reads==1u);
    port_v37_write32(irp+60u,0u);
    CHECK(fails_when(bad_func,&q));
    CHECK(reads==1u);
    port_v37_write16(irp+32u,PORT_DQ_READPBLK);
    CHECK(fails_when(bad_media,&q));
    port_v37_write32(irp+56u,1u);
    CHECK(fails_when(bad_status,&q));
    port_v37_write16(irp+42u,PORT_DQ_PHYSIO);
    q.read_sector=disk_error;
    CHECK(fails_when(0,&q));
    CHECK(reads==2u);
    q.read_sector=read_home;
    q.boot_unit=1u;
    CHECK(port_dq_bootread_execute(&q,&result)==PORT_DQ_BOOTREAD_BAD);
    q.boot_unit=0u;
    fdt.mask[PORT_DQ_FDT_LEGAL]^=PORT_DQ_BIT(12u);
    CHECK(port_dq_bootread_execute(&q,&result)==PORT_DQ_BOOTREAD_BAD);
    fdt.mask[PORT_DQ_FDT_LEGAL]^=PORT_DQ_BIT(12u);
    q.sector_bytes=510u;
    CHECK(port_dq_bootread_execute(&q,&result)==PORT_DQ_BOOTREAD_BAD);
    q.sector_bytes=512u;
    q.read_sector=0;
    CHECK(port_dq_bootread_execute(&q,&result)==PORT_DQ_BOOTREAD_BAD);
    puts("DQDRIVER.MAR privileged native read of real Files-11 LBN1, original IRP/UCB completion and fail-closed controls PASS");
    return 0;
}
