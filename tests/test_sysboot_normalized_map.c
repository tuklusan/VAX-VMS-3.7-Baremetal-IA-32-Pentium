#include "vms_sysboot_map.h"
#include "vms_boot_files11.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#define CHECK(x) do {if(!(x)){fprintf(stderr,"FAIL %s:%u: %s\n",__FILE__,__LINE__,#x);return 1;}} while(0)
static uint32_t get32(const uint8_t *p){
    return (uint32_t)p[0]|((uint32_t)p[1]<<8)|
           ((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);
}
struct disk {FILE *file;};
static uint32_t read_sector(void *ctx,uint32_t mode,uint32_t fn,
                            uint32_t lbn,uint32_t size,void *data){
    struct disk *d=ctx;
    if(mode || fn!=1u || size!=512u || !data ||
       fseek(d->file,(long)lbn*512L,SEEK_SET) ||
       fread(data,1,512u,d->file)!=512u)return BOO_IO_FAILED;
    return BOO_IO_NORMAL;
}
int main(int argc,char **argv){
    CHECK(argc==2);
    uint32_t memory[128];
    uint8_t *m=(uint8_t *)memory;
    struct port_f11_extent e[3]={{7u,128u},{10u,140u},{3u,160u}};
    struct port_sysboot_normalized_map out={0};
    memset(memory,0xa5,sizeof memory);
    CHECK(port_sysboot_build_normalized_map(m,20u,e,3u,20u,&out)==PORT_F11_FILE_BAD_MAP);
    for(size_t i=0;i<sizeof memory;i++)CHECK(m[i]==0xa5u);
    CHECK(port_sysboot_build_normalized_map(m,sizeof memory,e,3u,21u,&out)==PORT_F11_FILE_BAD_MAP);
    for(size_t i=0;i<sizeof memory;i++)CHECK(m[i]==0xa5u);
    CHECK(port_sysboot_build_normalized_map(m,sizeof memory,e,3u,20u,&out)==PORT_F11_HOME_OK);
    CHECK(get32(m)==24u && out.extent_count==3u && out.normalized_bytes==24u && out.total_blocks==20u);
    for(uint32_t i=0;i<3u;i++){
       CHECK(get32(m+4u+8u*i)==e[i].blocks);
       CHECK(get32(m+8u+8u*i)==e[i].lbn);
    }
    CHECK(m[28u]==0xa5u);
    struct port_sysboot_normalized_map second={0};
    CHECK(port_sysboot_check_normalized_map(m,sizeof memory,20u,&second)==PORT_F11_HOME_OK);
    CHECK(second.record==m && second.total_blocks==20u && second.extent_count==3u);
    m[0]=7u;
    CHECK(port_sysboot_check_normalized_map(m,sizeof memory,20u,&second)==PORT_F11_FILE_BAD_MAP);
    m[0]=24u;
    e[1].blocks=0u;
    memset(memory,0xa5,sizeof memory);
    CHECK(port_sysboot_build_normalized_map(m,sizeof memory,e,3u,20u,&out)==PORT_F11_FILE_BAD_MAP);
    for(size_t i=0;i<sizeof memory;i++)CHECK(m[i]==0xa5u);
    e[1].blocks=10u;
    e[0].lbn=UINT32_MAX;
    CHECK(port_sysboot_build_normalized_map(m,sizeof memory,e,3u,20u,&out)==PORT_F11_FILE_BAD_MAP);
    e[0].lbn=128u;
    CHECK(port_sysboot_build_normalized_map(m,sizeof memory,e,0u,20u,&out)==PORT_F11_HOME_BADARG);
    CHECK(port_sysboot_build_normalized_map(m+1u,sizeof memory-1u,e,3u,20u,&out)==PORT_F11_HOME_BADARG);
    struct disk d={fopen(argv[1],"rb")};CHECK(d.file);
    struct boo_io_driver disk={read_sector,&d,0u};
    struct port_f11_volume volume={0};
    CHECK(port_f11_mount(&disk,1u,&volume)==PORT_F11_HOME_OK);
    struct port_f11_fid fid={0};
    uint8_t header[512];
    uint32_t first_lbn=0u,alloc=0u;
    CHECK(port_f11_open_named_file(&disk,1u,&volume,"[SYSEXE]SYS.EXE",
       &fid,header,&first_lbn,&alloc)==PORT_F11_HOME_OK);
    CHECK(fid.number==11u && alloc>0u);
    struct port_f11_extent original[48]={0};
    uint32_t count=0u,blocks=0u,contiguous=0u;
    CHECK(port_f11_collect_file_extents(&disk,1u,&volume,fid,
       original,48u,&count,&blocks,&contiguous)==PORT_F11_HOME_OK);
    CHECK(count>0u && count<=48u && blocks>=alloc);
    CHECK(port_sysboot_build_normalized_map(m,sizeof memory,
       original,count,alloc,&out)==PORT_F11_HOME_OK);
    CHECK(port_sysboot_check_normalized_map(m,sizeof memory,
       alloc,&second)==PORT_F11_HOME_OK);
    CHECK(second.total_blocks==blocks && second.extent_count==count);
    for(uint32_t i=0;i<count;i++){
        CHECK(get32(m+4u+i*8u)==original[i].blocks);
        CHECK(get32(m+8u+i*8u)==original[i].lbn);
    }
    if(contiguous)CHECK(contiguous==first_lbn);
    fclose(d.file);
    puts("Native SYSBOOT guest BOO$L_SYS_MAP normalized real Files-11 SYS.EXE extents PASS");
    return 0;
}
