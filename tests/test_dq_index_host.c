#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "vms_dq_index_header.h"
#define CHECK(x) do {if(!(x)){fprintf(stderr,"DQ INDEXF host test line %d: %s\n",__LINE__,#x);return 1;}}while(0)
struct backend {FILE *file;uint32_t calls,last_lbn;int fail;};
static int read_disk(void *p,uint32_t lbn,uint16_t data[256]){
    struct backend *b=p;b->calls++;b->last_lbn=lbn;
    if(b->fail||fseek(b->file,(long)lbn*512L,SEEK_SET)!=0 ||
       fread(data,1,512,b->file)!=512)return 1;
    return 0;
}
static void sum(uint8_t b[512],size_t at){
    uint16_t s=0;
    for(size_t i=0;i<at;i+=2u)s=(uint16_t)(s+port_dq_index_word(b,i));
    b[at]=(uint8_t)s;b[at+1u]=(uint8_t)(s>>8);
}
static void fixhome(uint8_t b[512]){sum(b,58u);sum(b,510u);}
static void put16(uint8_t *b,size_t at,uint16_t v){b[at]=(uint8_t)v;b[at+1u]=(uint8_t)(v>>8);}
static void put32(uint8_t *b,size_t at,uint32_t v){put16(b,at,(uint16_t)v);put16(b,at+2u,(uint16_t)(v>>16));}
static uint32_t mutatehome(uint8_t b[512],int n){
    switch(n){
    case 0:b[13]=1u;break;
    case 1:put32(b,0u,0u);break;
    case 2:put32(b,0u,2u);break;
    case 3:put32(b,8u,0u);break;
    case 4:put16(b,14u,0u);break;
    case 5:put16(b,16u,0u);break;
    case 6:put16(b,18u,0u);break;
    case 7:put16(b,20u,0u);break;
    case 8:put16(b,22u,0u);break;
    case 9:put32(b,28u,0u);break;
    case 10:put16(b,34u,0u);break;
    case 11:put32(b,28u,1u);put16(b,34u,2u);break;
    case 12:put32(b,24u,0u);break;
    case 13:put16(b,32u,0u);break;
    case 14:put32(b,24u,UINT32_C(0x10000000));break;
    case 15:put32(b,24u,UINT32_C(0x0fffffff));put16(b,32u,1u);break;
    case 16:put32(b,24u,UINT32_MAX);break;
    default:return 0u;
    }
    fixhome(b);
    return 1u;
}
int main(void){
    uint32_t db[PORT_V37_DEVICE_BYTES/4u]={0};
    uint32_t irp[PORT_V37_IRP_SIZE/4u]={0};
    uint16_t sector[256]={0},home[256]={0};
    struct port_v37_device_result device={0};
    struct port_dq_fdt fdt={0};
    struct backend disk={0};
    disk.file=fopen("portdisk.img","rb");CHECK(disk.file);
    CHECK(fseek(disk.file,512L,SEEK_SET)==0);
    CHECK(fread(home,1,512,disk.file)==512u);
    uint32_t expected=0u;
    CHECK(port_dq_index_location((const uint8_t *)home,&expected)==PORT_DQ_INDEX_OK);
    CHECK(expected>1u&&expected<UINT32_C(0x10000000));
    CHECK(port_v37_devices_prelink((uint8_t *)db,sizeof db,(uint32_t)(uintptr_t)db,&device)==PORT_V37_DEV_OK);
    port_v37_write32((uint8_t *)db+PORT_V37_DDB_NL_OFF+12u,0x00700000u);
    port_v37_write32((uint8_t *)db+PORT_V37_UCB_NL_OFF+120u,0x00700000u);
    device.bound_nl_ddt=0x00700000u;device.incomplete_driver_mask=7u;
    CHECK(port_v37_devices_check((uint8_t *)db,sizeof db,(uint32_t)(uintptr_t)db,&device)==PORT_V37_DEV_OK);
    CHECK(port_dq_fdt_build(&fdt,sizeof fdt)==PORT_DQ_FDT_OK);
    CHECK(port_v37_irp_prepare((uint8_t *)irp,sizeof irp,(uint32_t)(uintptr_t)irp,device.ucb[0],PORT_DQ_READPBLK)==PORT_V37_QIO_OK);
    port_v37_write16((uint8_t *)irp+42u,PORT_DQ_PHYSIO);
    port_v37_write16((uint8_t *)irp+50u,512u);
    port_v37_write32((uint8_t *)irp+PORT_V37_IRP_MEDIA,expected);
    const struct port_dq_bootread_request q={
        .fdt=&fdt,.device_db=(uint8_t *)db,.device_info=&device,
        .irp=(uint8_t *)irp,.irp_bytes=sizeof irp,
        .sector=sector,.sector_bytes=sizeof sector,.boot_unit=0u,
        .read_sector=read_disk,.read_context=&disk};
    struct port_dq_bootread_result io={0};
    struct port_dq_index_result result={0};
    CHECK(port_dq_index_read(&q,home,&io,&result)==PORT_DQ_INDEX_OK);
    CHECK(disk.calls==1u&&disk.last_lbn==expected&&
          io.ucb_io_count==1u&&result.io_count==1u&&
          result.lbn==expected&&result.bitmap_lbn+result.bitmap_blocks==expected&&
          result.file_number==1u&&result.file_sequence==1u&&
          result.record_bytes==512u);
    /* Source mutation tests, including two independent checksum cells. */
    uint8_t mutant[512];
    for(int i=0;i<=16;i++){
        memcpy(mutant,home,512u);CHECK(mutatehome(mutant,i));
        uint32_t lbn=UINT32_C(0xcafebabe);
        CHECK(port_dq_index_location(mutant,&lbn)==PORT_DQ_INDEX_HOME);
        CHECK(lbn==UINT32_C(0xcafebabe));
    }
    memcpy(mutant,home,512u);mutant[58u]^=1u;
    CHECK(port_dq_index_location(mutant,&expected)==PORT_DQ_INDEX_HOME);
    memcpy(mutant,home,512u);mutant[510u]^=1u;
    CHECK(port_dq_index_location(mutant,&expected)==PORT_DQ_INDEX_HOME);
    CHECK(port_dq_index_location(NULL,&expected)==PORT_DQ_INDEX_BAD);
    CHECK(port_dq_index_location((const uint8_t *)home,NULL)==PORT_DQ_INDEX_BAD);
    /* Reject corrupt FH2 fields and checksum without accepting a false root. */
    for(unsigned i=0;i<8u;i++){
        uint8_t h[512];memcpy(h,sector,512u);
        switch(i){
        case 0:h[7u]=1u;break;
        case 1:put16(h,8u,2u);break;
        case 2:put16(h,10u,2u);break;
        case 3:put16(h,12u,1u);break;
        case 4:h[0u]=0u;break;
        case 5:h[1u]=h[0u];break;
        case 6:h[1u]=255u;break;
        default:h[510u]^=1u;break;
        }
        if(i!=7u)sum(h,510u);
        struct port_dq_index_result bad={.lbn=UINT32_C(0xdecafbad)};
        CHECK(port_dq_index_validate_header((const uint16_t *)h,&bad)==PORT_DQ_INDEX_HEADER);
        CHECK(bad.lbn==UINT32_C(0xdecafbad));
    }
    CHECK(port_dq_index_validate_header(NULL,&result)==PORT_DQ_INDEX_BAD);
    CHECK(port_dq_index_validate_header(sector,NULL)==PORT_DQ_INDEX_BAD);
    CHECK(port_dq_index_checksum(NULL,58u)==0);
    CHECK(port_dq_index_checksum((const uint8_t *)home,59u)==0);
    CHECK(port_dq_index_checksum((const uint8_t *)home,512u)==0);
    CHECK(port_dq_index_read(NULL,home,&io,&result)==PORT_DQ_INDEX_BAD);
    CHECK(port_dq_index_read(&q,NULL,&io,&result)==PORT_DQ_INDEX_BAD);
    CHECK(port_dq_index_read(&q,home,NULL,&result)==PORT_DQ_INDEX_BAD);
    CHECK(port_dq_index_read(&q,home,&io,NULL)==PORT_DQ_INDEX_BAD);
    CHECK(fclose(disk.file)==0);
    puts("FIL$MOUNT native resident INDEXF.SYS FH2 physical LBN and source structural checks PASS");
    return 0;
}
