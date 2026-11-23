#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "vms_init_index.h"
#include "vms_init_mfd.h"
#include "vms_init_bitmap.h"
#include "vms_boot_files11.h"
#include "vms_bootio.h"

static int failed;
#define CHECK(x) do {if(!(x)){fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x);failed++;}}while(0)
static uint8_t sectors[8192][512];
static unsigned writes[8192];
static uint32_t write_sector(void *ctx,uint32_t lbn,const uint8_t b[512]){
    (void)ctx;
    if(lbn>=8192u){failed++;return 2u;}
    memcpy(sectors[lbn],b,512);writes[lbn]++;
    return 1u;
}
static uint32_t read_sector(void *ctx,uint32_t mode,uint32_t fn,uint32_t lbn,
                            uint32_t size,void *out){
    (void)ctx;
    if(mode || fn!=1u || size!=512u || lbn>=8192u)return 2u;
    memcpy(out,sectors[lbn],512);return 1u;
}
static uint16_t get16(const uint8_t *b){return (uint16_t)(b[0]|(b[1]<<8));}
int main(void){
    static const struct port_init_allocation index_area[]={
        {0,3},{3,3},{6,3},{9,3},{12,12}
    };
    static const struct port_init_allocation occupied[]={
        {0,3},{3,3},{6,3},{9,3},{12,12},{24,3},{27,3}
    };
    struct port_init_index_layout o={0};
    o.cluster=3u;o.volume_blocks=8192u;o.max_files=128u;
    o.boot_lbn=0u;o.boot_blocks=3u;o.home1_lbn=3u;
    o.home2_lbn=6u;o.real_home_lbn=6u;
    o.alternate_index_lbn=9u;o.index_lbn=12u;o.index_blocks=12u;
    o.bitmap_lbn=24u;o.bitmap_blocks=3u;o.mfd_lbn=27u;o.mfd_blocks=3u;
    o.owner_uic=0x00010004u;o.file_protection=0xAA00u;
    o.creation_time=50000000000000000ull;
    memcpy(o.label,"LABDISK",8);
    memcpy(o.owner_name,"SYSTEM",7);
    o.index_areas=index_area;o.index_area_count=5u;
    uint8_t b[512];
    CHECK(port_init_index(&o,b,write_sector,0)==1u);
    CHECK(writes[0]==1u && sectors[0][510]==0x55u && sectors[0][511]==0xaau);
    for(unsigned j=1;j<9;j++)CHECK(writes[j]==1u);
    CHECK(writes[9]==1u && writes[12]==1u && writes[13]==1u);
    CHECK(writes[16]==1u && writes[21]==1u);
    CHECK(port_init_mfd(o.mfd_lbn,0u,b,write_sector,0)==1u);
    const struct port_init_bitmap_options bit={0,24,3,8192,3,63,16,8};
    CHECK(port_init_bitmap(&bit,occupied,7u,b,write_sector,0)==1u);
    CHECK(get16(sectors[1]+12)==513u);
    CHECK(get16(sectors[12])==0x1ffu);
    const struct boo_io_driver disk={read_sector,0,0};
    struct port_f11_volume vol={0};
    CHECK(port_f11_mount(&disk,1,&vol)==PORT_F11_HOME_OK);
    CHECK(vol.index_header_lbn==13u && vol.index_vbn_offset==13u);
    uint8_t h[512];
    const struct port_f11_fid mfd={4,4,0};
    CHECK(port_f11_read_file_header(&disk,1,&vol,mfd,h)==PORT_F11_HOME_OK);
    uint32_t lbn=0,count=0;
    CHECK(port_f11_file_extent_stats(h,&lbn,&count)==PORT_F11_HOME_OK);
    CHECK(lbn==27u&&count==3u);
    struct port_f11_fid found={0};
    CHECK(port_f11_find_file(&disk,1,&vol,mfd,"INDEXF.SYS",0,&found)==PORT_F11_HOME_OK);
    CHECK(found.number==1u && found.sequence==1u);
    CHECK(port_f11_find_file(&disk,1,&vol,mfd,"BITMAP.SYS",1,&found)==PORT_F11_HOME_OK);
    CHECK(found.number==2u && found.sequence==2u);
    CHECK(port_f11_find_file(&disk,1,&vol,mfd,"000000.DIR",1,&found)==PORT_F11_HOME_OK);
    CHECK(found.number==4u && found.sequence==4u);
    CHECK(port_f11_open_named_file(&disk,1,&vol,"SYS.EXE",&found,h,&lbn,&count)==PORT_F11_FILE_NOT_FOUND);
    if(failed)return 1;
    puts("PASS: original INIT_INDEX+INIT_MFD+INIT_BITMAP build ODS-2 volume read by FIL$MOUNT");
    return 0;
}
