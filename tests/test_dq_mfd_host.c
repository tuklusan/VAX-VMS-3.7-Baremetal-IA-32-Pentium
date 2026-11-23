#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "vms_dq_mfd_header.h"
#define CHECK(x) do {if(!(x)){fprintf(stderr,"MFD host line %d: %s\n",__LINE__,#x);return 1;}}while(0)
struct disk {FILE *f;uint32_t calls,last;};
static int real_lbn(void *arg,uint32_t lbn,uint16_t block[256]){
    struct disk *d=arg;d->calls++;d->last=lbn;
    return fseek(d->f,512L*(long)lbn,SEEK_SET)||
        fread(block,1,512u,d->f)!=512u;
}
static void set16(uint8_t *b,unsigned at,uint16_t n){b[at]=(uint8_t)n;b[at+1u]=(uint8_t)(n>>8);}
static void fixsum(uint8_t *b){uint16_t s=0;for(unsigned i=0;i<510u;i+=2u)s=(uint16_t)(s+port_dq_index_word(b,i));set16(b,510u,s);}
int main(void){
    struct disk d={.f=fopen("portdisk.img","rb")};CHECK(d.f);
    uint16_t home[256]={0},index[256]={0},sector[256]={0};
    CHECK(fseek(d.f,512L,SEEK_SET)==0);
    CHECK(fread(home,1,512u,d.f)==512u);
    uint32_t index_lbn=0u;
    CHECK(port_dq_index_location((const uint8_t *)home,&index_lbn)==PORT_DQ_INDEX_OK);
    CHECK(fseek(d.f,(long)index_lbn*512L,SEEK_SET)==0);
    CHECK(fread(index,1,512u,d.f)==512u);
    uint32_t mfd_vbn=0u,mfd_lbn=0u;
    CHECK(port_dq_mfd_location((const uint8_t *)home,(const uint8_t *)index,
                              &mfd_vbn,&mfd_lbn)==PORT_DQ_MFD_OK);
    CHECK(mfd_vbn>4u && mfd_lbn>1u);
    uint32_t db[PORT_V37_DEVICE_BYTES/4u]={0};
    uint32_t irp[PORT_V37_IRP_SIZE/4u]={0};
    struct port_v37_device_result info={0};
    struct port_dq_fdt fdt={0};
    const uint32_t base=(uint32_t)(uintptr_t)db;
    CHECK(port_v37_devices_prelink((uint8_t *)db,sizeof db,base,&info)==PORT_V37_DEV_OK);
    port_v37_write32((uint8_t *)db+PORT_V37_DDB_NL_OFF+12u,0x00700000u);
    port_v37_write32((uint8_t *)db+PORT_V37_UCB_NL_OFF+120u,0x00700000u);
    info.bound_nl_ddt=0x00700000u;info.incomplete_driver_mask=7u;
    CHECK(port_v37_devices_check((uint8_t *)db,sizeof db,base,&info)==PORT_V37_DEV_OK);
    CHECK(port_dq_fdt_build(&fdt,sizeof fdt)==PORT_DQ_FDT_OK);
    CHECK(port_v37_irp_prepare((uint8_t *)irp,sizeof irp,(uint32_t)(uintptr_t)irp,
                  info.ucb[0],PORT_DQ_READPBLK)==PORT_V37_QIO_OK);
    port_v37_write16((uint8_t *)irp+42u,PORT_DQ_PHYSIO);
    port_v37_write16((uint8_t *)irp+50u,512u);
    port_v37_write32((uint8_t *)irp+PORT_V37_IRP_MEDIA,mfd_lbn);
    struct port_dq_bootread_request q={.fdt=&fdt,.device_db=(uint8_t *)db,
        .device_info=&info,.irp=(uint8_t *)irp,.irp_bytes=sizeof irp,
        .sector=sector,.sector_bytes=sizeof sector,.boot_unit=0u,
        .read_sector=real_lbn,.read_context=&d};
    struct port_dq_bootread_result io={0};
    struct port_dq_mfd_result mfd={0};
    CHECK(port_dq_mfd_read(&q,home,index,&io,&mfd)==PORT_DQ_MFD_OK);
    CHECK(d.calls==1u&&d.last==mfd_lbn && mfd.lbn==mfd_lbn&&
        mfd.vbn==mfd_vbn&&mfd.io_count==1u &&mfd.fid==4u&&mfd.sequence==4u);
    CHECK(port_dq_mfd_validate(sector,&mfd)==PORT_DQ_MFD_OK);
    uint8_t bad[512];
    for(unsigned i=0;i<8u;i++){
        memcpy(bad,sector,sizeof bad);
        switch(i){
        case 0:bad[7u]=1u;break;
        case 1:set16(bad,8u,5u);break;
        case 2:set16(bad,10u,5u);break;
        case 3:set16(bad,12u,1u);break;
        case 4:bad[0u]=0u;break;
        case 5:bad[1u]=bad[0u];break;
        case 6:bad[1u]=255u;break;
        default:bad[510u]^=1u;break;
        }
        if(i!=7u)fixsum(bad);
        struct port_dq_mfd_result out={.lbn=0xaabbccddu};
        CHECK(port_dq_mfd_validate((const uint16_t *)bad,&out)==PORT_DQ_MFD_HEADER);
        CHECK(out.lbn==0xaabbccddu);
    }
    CHECK(port_dq_mfd_validate(NULL,&mfd)==PORT_DQ_MFD_BAD);
    CHECK(port_dq_mfd_validate(sector,NULL)==PORT_DQ_MFD_BAD);
    for(unsigned i=0;i<7u;i++){
        memcpy(bad,index,sizeof bad);
        switch(i){
        case 0:bad[7u]=1u;break;
        case 1:set16(bad,8u,2u);break;
        case 2:set16(bad,10u,2u);break;
        case 3:set16(bad,12u,1u);break;
        case 4:bad[1u]=bad[0u];break;
        case 5:bad[58u]=255u;break;
        default:bad[510u]^=1u;break;
        }
        if(i!=6u)fixsum(bad);
        uint32_t vn=0xfefeu,lb=0xfefeu;
        CHECK(port_dq_mfd_location((const uint8_t *)home,bad,&vn,&lb)!=PORT_DQ_MFD_OK);
        CHECK(vn==0xfefeu&&lb==0xfefeu);
    }
    CHECK(port_dq_mfd_location(NULL,(const uint8_t *)index,&mfd_vbn,&mfd_lbn)==PORT_DQ_MFD_BAD);
    CHECK(port_dq_mfd_location((const uint8_t *)home,NULL,&mfd_vbn,&mfd_lbn)==PORT_DQ_MFD_BAD);
    CHECK(port_dq_mfd_location((const uint8_t *)home,(const uint8_t *)index,NULL,&mfd_lbn)==PORT_DQ_MFD_BAD);
    CHECK(port_dq_mfd_location((const uint8_t *)home,(const uint8_t *)index,&mfd_vbn,NULL)==PORT_DQ_MFD_BAD);
    CHECK(port_dq_mfd_read(NULL,home,index,&io,&mfd)==PORT_DQ_MFD_BAD);
    CHECK(port_dq_mfd_read(&q,NULL,index,&io,&mfd)==PORT_DQ_MFD_BAD);
    CHECK(port_dq_mfd_read(&q,home,NULL,&io,&mfd)==PORT_DQ_MFD_BAD);
    CHECK(port_dq_mfd_read(&q,home,index,NULL,&mfd)==PORT_DQ_MFD_BAD);
    CHECK(port_dq_mfd_read(&q,home,index,&io,NULL)==PORT_DQ_MFD_BAD);
    CHECK(fclose(d.f)==0);
    puts("FIL$READVBN/FIL$RDCHKFILHDR native MFD FID 4,4,0 physical disk path PASS");
    return 0;
}
