#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "vms_bootio.h"
#include "vms_boot_files11.h"
#include "vms_sysboot_read.h"
static int trouble;
#define CHECK(t) do {if(!(t)){fprintf(stderr,"FAIL: %s:%d: %s\n",__FILE__,__LINE__,#t);trouble++;}} while(0)
static uint32_t last_mode,last_func,last_lbn,last_size;
static uint32_t io(void *ctx,uint32_t mode,uint32_t func,
                   uint32_t lbn,uint32_t count,void *dst){
    (void)ctx; last_mode=mode;last_func=func;last_lbn=lbn;last_size=count;
    if(dst && count) memset(dst,0x5a,count);
    return BOO_IO_NORMAL;
}
static uint32_t cache_init(void *ctx,uint32_t bytes,uint32_t addr,
                            uint32_t blocks,uint32_t entries,
                            uint32_t *end_bytes,uint32_t *end_addr){
    (void)ctx;
    CHECK(bytes==64u*512u);CHECK(addr==2048u*512u);
    CHECK(blocks==62u);CHECK(entries==47u);
    *end_bytes=bytes;*end_addr=addr;
    return BOO_IO_NORMAL;
}
static void set32(uint8_t *p,uint32_t v){
    p[0]=(uint8_t)v;p[1]=(uint8_t)(v>>8);
    p[2]=(uint8_t)(v>>16);p[3]=(uint8_t)(v>>24);
}
static void set16(uint8_t *p,uint16_t v){p[0]=(uint8_t)v;p[1]=(uint8_t)(v>>8);}
struct test_volume_blocks { uint8_t home[512]; uint8_t index[512]; uint8_t file[512]; uint8_t dirhdr[512]; uint8_t dirdata[512];
                            uint8_t subhdr[512]; uint8_t subdata[512];
                            uint8_t sys_image[512]; uint8_t sys_params[512]; uint8_t ext_header[512]; uint8_t ext_data[512]; };
static uint32_t disk_read(void *ctx,uint32_t mode,uint32_t func,
                          uint32_t lbn,uint32_t count,void *buf) {
    struct test_volume_blocks *d=ctx;
    if(mode||func!=1||count!=512) return BOO_IO_FAILED;
    if(lbn==1u) memcpy(buf,d->home,512);
    else if(lbn==82u)memcpy(buf,d->index,512);
    else if(lbn==115u)memcpy(buf,d->file,512);
    else if(lbn==117u)memcpy(buf,d->dirhdr,512);
    else if(lbn==300u)memcpy(buf,d->dirdata,512);
    else if(lbn==118u)memcpy(buf,d->subhdr,512);
    else if(lbn==301u)memcpy(buf,d->subdata,512);
    else if(lbn==116u)memcpy(buf,d->ext_header,512);
    else if(lbn==400u)memcpy(buf,d->ext_data,512);
    else if(lbn==200u)memcpy(buf,d->sys_image,512);
    else if(lbn==201u)memcpy(buf,d->sys_params,512);
    else return BOO_IO_FAILED;
    return BOO_IO_NORMAL;
}
static void checksum(uint8_t *b,unsigned at) {
    uint16_t s=0;
    for(unsigned j=0;j<at;j+=2)s=(uint16_t)(s+(uint16_t)(b[j]|(b[j+1]<<8)));
    set16(b+at,s);
}
int main(void) {
    CHECK(boo_default_directory[0]==8u &&
          memcmp(boo_default_directory+1,"[SYSEXE]",8u)==0);
    CHECK(boo_default_device[0]==0u);
    const struct boo_io_driver d={io,0,0x12345678};
    uint8_t b[512];uint32_t ch=0;
    CHECK(boo_assign(&d,&ch)==BOO_IO_NORMAL && ch==0x12345678u);
    CHECK(boo_rdwr_lbn(&d,14,b,64,512)==BOO_IO_NORMAL);
    CHECK(last_mode==0 && last_func==64 && last_lbn==14 && last_size==512 && b[511]==0x5a);
    CHECK(boo_rdwr_lbn(0,14,b,0,512)==BOO_IO_BADPARAM);
    CHECK(boo_rdwr_lbn(&d,14,0,0,512)==BOO_IO_BADPARAM);
    uint8_t bitmap[8192]={0};
    for(unsigned p=0;p<16384;++p) bitmap[p>>3]|=(uint8_t)(1u<<(p&7));
    struct boo_page_map map={bitmap,sizeof bitmap,0,16384};
    struct boo_cache cache;
    boo_cache_alloc(&map,&cache);
    CHECK(cache.page_count==64u && cache.start_pfn==2048u);
    struct boo_cache_service c={cache_init,0,47};
    map.total_pages=14800;
    boo_cache_alloc(&map,&cache);
    CHECK(cache.page_count==64u && cache.start_pfn==2048u);
    map.total_pages=16384;
    CHECK(boo_cache_open(&cache,&c)==BOO_IO_NORMAL);
    /* A bad bitmap cannot claim free memory. */
    memset(bitmap+256,0,64);
    boo_cache_alloc(&map,&cache);
    CHECK(cache.page_count==0);
    map.total_pages=0;
    boo_cache_alloc(&map,&cache);
    CHECK(cache.page_count==0);
    /* BOO$IMAGE_ATT keeps the last VBN preceding debugging and patch data. */
    memset(b,0,sizeof b);
    const struct boo_image_layout layout={4,6,8,0,4,0};
    set16(b+4,32);set16(b+6,60);b[8]=2;
    set32(b+32,118);set32(b+36,100);set32(b+60,88);
    uint32_t last=160,headers=0;
    CHECK(boo_image_attributes(b,sizeof b,&layout,&last,&headers));
    CHECK(last==87 && headers==2);
    set32(b+60,0);last=160;
    CHECK(boo_image_attributes(b,sizeof b,&layout,&last,&headers));
    CHECK(last==99);
    /* HM2 byte offsets from original F11DEF.B32. */
    memset(b,0,sizeof b);
    struct port_f11_home home;
    CHECK(port_f11_home_read(b,512,1,&home)==PORT_F11_HOME_NOT_ODS2);
    set16(b+12,513);set32(b,1);set32(b+8,85);set16(b+14,3);
    set16(b+16,1);set16(b+18,2);set16(b+20,3);set16(b+22,4);
    set32(b+24,80);set32(b+28,128);set16(b+32,2);set16(b+34,16);
    uint16_t sum=0;
    for(unsigned j=0;j<58;j+=2)sum=(uint16_t)(sum+(uint16_t)(b[j]|(b[j+1]<<8)));
    set16(b+58,sum);sum=0;
    for(unsigned j=0;j<510;j+=2)sum=(uint16_t)(sum+(uint16_t)(b[j]|(b[j+1]<<8)));
    set16(b+510,sum);
    CHECK(port_f11_home_read(b,512,1,&home)==PORT_F11_HOME_OK);
    CHECK(home.cluster==3 && home.index_bitmap_lbn==80 && home.reserved_files==16);
    CHECK(port_f11_home_read(b,512,2,&home)==PORT_F11_HOME_BAD_FIELDS);
    struct test_volume_blocks vol={0};
    memcpy(vol.home,b,512);
    uint8_t *idx=vol.index;
    idx[0]=40; idx[1]=100; set16(idx+6,513);
    set16(idx+8,1);set16(idx+10,1);
    set16(idx+200,0x807fu);set32(idx+202,100);
    idx[58]=3;
    checksum(idx,510);
    uint8_t *h=vol.file;
    h[0]=40;h[1]=100;h[58]=2;
    set16(h+6,513);set16(h+8,2);set16(h+10,1);
    set16(h+200,0x4001u);
    set16(h+202,200u);
    checksum(h,510);
    const struct boo_io_driver vd={disk_read,&vol,0};
    struct port_f11_volume mounted;
    CHECK(port_f11_mount(&vd,1,&mounted)==PORT_F11_HOME_OK);
    CHECK(mounted.index_header_lbn==82u && mounted.index_vbn_offset==14u);
    uint32_t sector=0,first=0,count=0;
    CHECK(port_f11_vbn_to_lbn(mounted.index_header,16,&sector)==PORT_F11_HOME_OK);
    CHECK(sector==115u);
    uint8_t loaded[512];
    const struct port_f11_fid fid={2,1,0};
    CHECK(port_f11_read_file_header(&vd,1,&mounted,fid,loaded)==PORT_F11_HOME_OK);
    CHECK(port_f11_file_extent_stats(loaded,&first,&count)==PORT_F11_HOME_OK);
    CHECK(first==200u && count==2u);
    CHECK(port_f11_vbn_to_lbn(loaded,3,&sector)==PORT_F11_FILE_EOF);
    loaded[10]^=1u;
    CHECK(port_f11_file_extent_stats(loaded,&first,&count)==PORT_F11_HOME_OK);
    struct port_f11_fid bad={2,2,0};
    CHECK(port_f11_read_file_header(&vd,1,&mounted,bad,loaded)==PORT_F11_FILE_BAD_SEQ);
    uint8_t *dir=vol.dirhdr;
    dir[0]=40;dir[1]=100;dir[58]=3;
    set16(dir+6,513);set16(dir+8,4);set16(dir+10,4);
    set32(dir+52,1u<<13);dir[20]=2;dir[21]=8;
    set32(dir+28,2u<<16);set16(dir+32,0);
    set16(dir+200,0x8000u);set32(dir+202,300);
    checksum(dir,510);
    uint8_t *record=vol.dirdata;
    set16(record,24);record[5]=10;
    memcpy(record+6,"SYSEXE.DIR",10);
    set16(record+16,1);set16(record+18,5);set16(record+20,1);
    set16(record+26,0xffffu);
    uint8_t *sub=vol.subhdr;
    sub[0]=40;sub[1]=100;sub[58]=3;
    set16(sub+6,513);set16(sub+8,5);set16(sub+10,1);
    set32(sub+52,1u<<13);sub[20]=2;sub[21]=8;
    set32(sub+28,2u<<16);set16(sub+32,0);
    set16(sub+200,0x8000u);set32(sub+202,301);
    checksum(sub,510);
    uint8_t *srec=vol.subdata;
    set16(srec,20);srec[5]=7;
    memcpy(srec+6,"SYS.EXE",7);
    set16(srec+14,1);set16(srec+16,2);set16(srec+18,1);
    set16(srec+22,0xffffu);
    struct port_f11_fid found={0};
    const struct port_f11_fid mfd={4,4,0};
    CHECK(port_f11_find_file(&vd,1,&mounted,mfd,"SYSEXE.DIR",1,&found)==PORT_F11_HOME_OK);
    CHECK(found.number==5 && found.sequence==1);
    struct port_f11_fid opened={0};
    CHECK(port_f11_open_named_file(&vd,1,&mounted,
         "SYS$SYSDEVICE:[SYSEXE]SYS.EXE;1",&opened,loaded,&first,&count)==PORT_F11_HOME_OK);
    CHECK(opened.number==2 && first==200u && count==2u);
    CHECK(port_f11_open_named_file(&vd,1,&mounted,
         "SYS.EXE",&opened,loaded,&first,&count)==PORT_F11_HOME_OK);
    CHECK(port_f11_open_named_file(&vd,1,&mounted,
         "SYS.EXE;BAD",&opened,loaded,&first,&count)==PORT_F11_HOME_BADARG);
    vol.sys_image[8]=1;
    for(unsigned k=0;k<512;k++)vol.sys_params[k]=(uint8_t)(k*3u+7u);
    struct port_sysboot_param_location location={{4,6,8,0,4,0},0,200};
    struct port_sysboot_read_result result={0};
    uint8_t params[512]={0};
    CHECK(port_sysboot_getcurrent(&vd,1,&mounted,"SYS.EXE",
          &location,params,sizeof params,&result)==PORT_F11_HOME_OK);
    CHECK(result.sys_file.number==2 && result.image_vbn==2 &&
          result.sysparam_vbn==2 && result.last_image_vbn==2);
    CHECK(memcmp(params,vol.sys_params,200)==0);
    uint8_t loaded_image[512]; uint32_t loaded_len=0;
    uint8_t original_header[512];
    const struct boo_image_layout image_layout={4,6,8,0,4,0};
    CHECK(port_sysboot_image_init(&vd,1,&mounted,"SYS.EXE",&image_layout,
          original_header,&result)==PORT_F11_HOME_OK);
    CHECK(result.image_vbn==2u && result.last_image_vbn==2u);
    CHECK(port_sysboot_load_code(&vd,1,original_header,&result,
          loaded_image,sizeof loaded_image,&loaded_len)==PORT_F11_HOME_OK);
    CHECK(loaded_len==512u && memcmp(loaded_image,vol.sys_params,512u)==0);
    CHECK(port_sysboot_load_code(&vd,1,original_header,&result,
          loaded_image,511u,&loaded_len)==PORT_SYSBOOT_SMALL_BUFFER);
    CHECK(port_sysboot_read_virtual(&vd,1,original_header,2u,200u,
          loaded_image,200u)==PORT_F11_HOME_OK);
    CHECK(memcmp(loaded_image,vol.sys_params,200u)==0);
    CHECK(port_sysboot_read_virtual(&vd,1,original_header,2u,513u,
          loaded_image,512u)==PORT_SYSBOOT_SMALL_BUFFER);
    CHECK(port_sysboot_read_virtual(&vd,1,original_header,3u,512u,
          loaded_image,512u)==PORT_F11_FILE_EOF);
    CHECK(port_sysboot_image_init(&vd,1,&mounted,"MISSING.EXE",&image_layout,
          original_header,&result)==PORT_F11_FILE_NOT_FOUND);
    /* An actual file may use extension file headers. FIL$RDCHKFILHDR
     * must follow FH2$W_EXT_FID and join both normalized maps. */
    set16(vol.file+14u,3u);set16(vol.file+16u,1u);
    checksum(vol.file,510u);
    memcpy(vol.ext_header,vol.file,512u);
    set16(vol.ext_header+8u,3u);
    vol.ext_header[58u]=2u;
    set16(vol.ext_header+200u,0x4000u);
    set16(vol.ext_header+202u,400u);
    set16(vol.ext_header+14u,0u);set16(vol.ext_header+16u,0u);
    checksum(vol.ext_header,510u);
    for(unsigned i=0;i<512u;i++)vol.ext_data[i]=(uint8_t)(i*7u+1u);
    struct port_f11_extent collected[4];uint32_t used=0u,total=0u,first_extent=0u;
    CHECK(port_f11_collect_file_extents(&vd,1u,&mounted,
          (struct port_f11_fid){2u,1u,0u},collected,4u,&used,
          &total,&first_extent)==PORT_F11_HOME_OK);
    CHECK(used==2u && total==3u && first_extent==0u &&
          collected[0].blocks==2u && collected[0].lbn==200u &&
          collected[1].blocks==1u && collected[1].lbn==400u);
    CHECK(port_sysboot_image_init(&vd,1u,&mounted,"SYS.EXE",&image_layout,
          original_header,&result)==PORT_F11_HOME_OK);
    CHECK(result.last_image_vbn==3u && result.extent_count==2u);
    uint8_t both[1024];loaded_len=0;
    CHECK(port_sysboot_load_code(&vd,1u,original_header,&result,
          both,sizeof both,&loaded_len)==PORT_F11_HOME_OK);
    CHECK(loaded_len==1024u &&
          memcmp(both,vol.sys_params,512u)==0 &&
          memcmp(both+512u,vol.ext_data,512u)==0);
    CHECK(port_sysboot_getcurrent(&vd,1u,&mounted,"SYS.EXE",&location,
          params,sizeof params,&result)==PORT_F11_HOME_OK);
    location.sysparam_image_offset=512u;
    CHECK(port_sysboot_getcurrent(&vd,1u,&mounted,"SYS.EXE",&location,
          params,sizeof params,&result)==PORT_F11_HOME_OK);
    CHECK(memcmp(params,vol.ext_data,200u)==0);
    /* No return to an earlier header is allowed: reject a cycle. */
    set16(vol.ext_header+14u,2u);set16(vol.ext_header+16u,1u);
    checksum(vol.ext_header,510u);
    CHECK(port_f11_collect_file_extents(&vd,1u,&mounted,
          (struct port_f11_fid){2u,1u,0u},collected,4u,&used,
          &total,&first_extent)==PORT_F11_FILE_BAD_MAP);
    set16(vol.ext_header+14u,0u);set16(vol.ext_header+16u,0u);
    checksum(vol.ext_header,510u);
    set16(vol.file+14u,0u);set16(vol.file+16u,0u);
    checksum(vol.file,510u);
    location.sysparam_image_offset=512;
    CHECK(port_sysboot_getcurrent(&vd,1,&mounted,"SYS.EXE",
          &location,params,sizeof params,&result)==PORT_SYSBOOT_BAD_PARAMETERS);
    CHECK(port_f11_find_file(&vd,1,&mounted,(struct port_f11_fid){5,1,0},"SYS.EXE",2,&found)==PORT_F11_FILE_NOT_FOUND);
    CHECK(port_f11_find_file(&vd,1,&mounted,mfd,"NO.EXE",0,&found)==PORT_F11_FILE_NOT_FOUND);
    vol.index[510]^=2;
    CHECK(port_f11_mount(&vd,1,&mounted)==PORT_F11_MOUNT_BAD_INDEX);
    b[80]^=0x01;
    CHECK(port_f11_home_read(b,512,1,&home)==PORT_F11_HOME_BAD_CHECKSUM);
    if(trouble) return 1;
    puts("PASS: BOOTIO original disk I/O, cache selection/allocation, image header; Files-11 home validation");
    return 0;
}
