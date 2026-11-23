#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "vms_dqddt.h"
#include "vms_ini_devicedb.h"
#define CHECK(x) do {if(!(x)){fprintf(stderr,"DQ DDT failed line %u: %s\n",(unsigned)__LINE__,#x);return 1;}}while(0)
static int mock_read(void *ctx,uint32_t lbn,uint16_t dst[256]){
    unsigned *calls=ctx;(*calls)++;
    for(unsigned i=0;i<256u;i++)dst[i]=(uint16_t)(lbn+i);
    return 0;
}
static int mock_write(void *ctx,uint32_t lbn,const uint16_t src[256]){
    unsigned *calls=ctx;(*calls)++;
    return lbn==7u&&src[0]==0x55aau&&src[255]==0xaa55u?0:-1;
}
int main(void){
    enum {BASE=0x00370000u,START=0x00324120u,RET=0x00324104u};
    const uint32_t action=(uint32_t)(uintptr_t)port_dqddt_bootread;
    const uint32_t write_action=(uint32_t)(uintptr_t)port_dqddt_bootwrite;
    struct port_dq_fdt fdt={0};
    CHECK(port_dq_fdt_build(&fdt,sizeof fdt)==PORT_DQ_FDT_OK);
    struct port_dqddt_stage d={0};
    CHECK(port_dqddt_stage_init(&d,&fdt,BASE,START,RET,action,write_action)==PORT_DQDDT_OK);
    CHECK(d.table_base==BASE&&d.native_start_pc==START&&d.ioc_return_pc==RET);
    CHECK(port_dqddt_validate(&d,&fdt)==PORT_DQDDT_OK);
    CHECK(port_v37_read32(d.image)==(uint32_t)(START-BASE));
    CHECK(port_v37_read32(d.image+4u)==RET);
    CHECK(port_v37_read32(d.image+8u)==PORT_DQDDT_HEADER_BYTES);
    CHECK(port_v37_read16(d.image+20u)==84u);
    CHECK(port_v37_read16(d.image+36u)==136u);
    CHECK(port_dqddt_entry_offset(0u)==40u&&port_dqddt_entry_offset(1u)==48u);
    CHECK(port_dqddt_entry_offset(2u)==56u&&port_dqddt_entry_offset(11u)==164u);
    CHECK(port_dqddt_entry_offset(12u)==UINT32_MAX);
    CHECK(port_v37_read32(d.image+port_dqddt_entry_offset(2u))==(uint32_t)fdt.mask[2u]);
    CHECK(port_v37_read32(d.image+port_dqddt_entry_offset(2u)+4u)==(uint32_t)(fdt.mask[2u]>>32u));
    CHECK(port_v37_read32(d.image+port_dqddt_entry_offset(2u)+8u)==
          (action|UINT32_C(0x80000000)));
    CHECK(port_v37_read32(d.image+port_dqddt_entry_offset(PORT_DQ_FDT_WRITE)+8u)==
          (write_action|UINT32_C(0x80000000)));
    CHECK(port_dqddt_function(&d,&fdt,PORT_DQ_READPBLK)==PORT_DQDDT_NATIVE);
    CHECK(port_dqddt_function(&d,&fdt,PORT_DQ_WRITEPBLK)==PORT_DQDDT_NATIVE);
    CHECK(port_dqddt_function(&d,&fdt,PORT_DQ_WRITELBLK)==PORT_DQDDT_UNBOUND);
    CHECK(port_dqddt_function(&d,&fdt,PORT_DQ_READVBLK)==PORT_DQDDT_UNBOUND);
    CHECK(port_dqddt_function(&d,&fdt,63u)==PORT_DQDDT_BAD);
    CHECK(port_dqddt_function(&d,&fdt,UINT16_MAX)==PORT_DQDDT_BAD);
    for(unsigned i=0;i<PORT_DQ_FDT_ROWS;i++){
        uint32_t pos=port_dqddt_entry_offset(i);
        uint8_t old=d.image[pos];d.image[pos]^=1u;
        CHECK(port_dqddt_validate(&d,&fdt)==PORT_DQDDT_BAD);
        d.image[pos]=old;
        if(i>=2u){old=d.image[pos+8u];d.image[pos+8u]=1u;
            CHECK(port_dqddt_validate(&d,&fdt)==PORT_DQDDT_BAD);
            d.image[pos+8u]=old;}
    }
    CHECK(port_dqddt_validate(&d,&fdt)==PORT_DQDDT_OK);
    d.image[20u]=0u;
    CHECK(port_dqddt_validate(&d,&fdt)==PORT_DQDDT_BAD);
    d.image[20u]=PORT_DQDDT_SOURCE_DIAG_BYTES;
    d.image[32u]=1u;
    CHECK(port_dqddt_validate(&d,&fdt)==PORT_DQDDT_BAD);
    d.image[32u]=0u;
    d.driver_bound=1u;
    CHECK(port_dqddt_validate(&d,&fdt)==PORT_DQDDT_BAD);
    d.driver_bound=0u;
    CHECK(port_dqddt_validate(&d,&fdt)==PORT_DQDDT_OK);

    /* Exercise both staged private physical action slots. */
    uint32_t db_words[PORT_V37_DEVICE_BYTES/4u]={0};
    uint32_t irp_words[PORT_V37_IRP_SIZE/4u]={0};
    uint16_t sector[256]={0};
    struct port_v37_device_result info={0};
    uint8_t *db=(uint8_t *)db_words,*irp=(uint8_t *)irp_words;
    CHECK(port_v37_devices_prelink(db,sizeof db_words,
        (uint32_t)(uintptr_t)db,&info)==PORT_V37_DEV_OK);
    port_v37_write32(db+PORT_V37_DDB_NL_OFF+12u,0x00700000u);
    port_v37_write32(db+PORT_V37_UCB_NL_OFF+120u,0x00700000u);
    info.bound_nl_ddt=0x00700000u;info.incomplete_driver_mask=7u;
    db[20u]=3u;db[21u]='D';db[22u]='Q';db[23u]='A';db[36u]=8u;
    {const char name[]="DQDRIVER";for(unsigned i=0;i<8u;i++)db[37u+i]=(uint8_t)name[i];}
    info.boot_name_ready=1u;
    CHECK(port_v37_devices_check(db,sizeof db_words,
        (uint32_t)(uintptr_t)db,&info)==PORT_V37_DEV_OK);
    CHECK(port_v37_irp_prepare(irp,PORT_V37_IRP_SIZE,0x00712000u,
        info.ucb[0],PORT_DQ_READPBLK)==PORT_V37_QIO_OK);
    port_v37_write16(irp+42u,PORT_DQ_PHYSIO);
    port_v37_write16(irp+50u,PORT_DQ_BOOTREAD_BYTES);
    port_v37_write32(irp+PORT_V37_IRP_MEDIA,7u);
    unsigned calls=0u;
    struct port_dq_bootread_request request={.fdt=&fdt,.device_db=db,
        .device_info=&info,.irp=irp,.irp_bytes=PORT_V37_IRP_SIZE,
        .sector=sector,.sector_bytes=sizeof sector,.boot_unit=0u,
        .read_sector=mock_read,.write_sector=mock_write,.read_context=&calls};
    struct port_dq_bootread_result read_result={0};
    CHECK(port_dqddt_dispatch_bootread(&d,&fdt,&request,&read_result,7u)==
        PORT_DQ_BOOTREAD_OK);
    CHECK(calls==1u&&read_result.lbn==7u&&read_result.bytes==512u&&
        read_result.irp_status==1u&&sector[0]==7u&&sector[255]==262u);
    CHECK(port_dqddt_dispatch_bootread(&d,&fdt,&request,&read_result,8u)==
        PORT_DQ_BOOTREAD_BAD);
    CHECK(calls==1u);
    memset(irp,0,PORT_V37_IRP_SIZE);
    CHECK(port_v37_irp_prepare(irp,PORT_V37_IRP_SIZE,0x00712000u,
        info.ucb[0],PORT_DQ_WRITEPBLK)==PORT_V37_QIO_OK);
    port_v37_write16(irp+42u,PORT_DQ_PHYSIO);
    port_v37_write16(irp+50u,PORT_DQ_BOOTREAD_BYTES);
    port_v37_write32(irp+PORT_V37_IRP_MEDIA,7u);
    sector[0]=0x55aau;sector[255]=0xaa55u;
    uint32_t write_status=port_dqddt_dispatch_bootwrite(&d,&fdt,&request,
        &read_result,7u);
    CHECK(write_status==PORT_DQ_BOOTREAD_OK);
    CHECK(calls==2u&&read_result.io_function==PORT_DQ_WRITEPBLK&&
        read_result.irp_status==1u&&read_result.bytes==512u);
    struct port_dqddt_stage before=d;
    CHECK(port_dqddt_stage_init(&d,&fdt,BASE+1u,START,RET,action,write_action)==PORT_DQDDT_BAD);
    CHECK(!memcmp(&before,&d,sizeof d));
    CHECK(port_dqddt_stage_init(&d,&fdt,BASE,BASE+4u,RET,action,write_action)==PORT_DQDDT_BAD);
    CHECK(!memcmp(&before,&d,sizeof d));
    CHECK(port_dqddt_stage_init(&d,&fdt,BASE,START,BASE+8u,action,write_action)==PORT_DQDDT_BAD);
    CHECK(!memcmp(&before,&d,sizeof d));
    puts("DQ DDT staged privileged READPBLK/WRITEPBLK dispatch PASS; general DDT and public QIO remain unbound");
    return 0;
}
