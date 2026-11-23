#include "vms_ini_syswcb.h"
#include "vms_boot_files11.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"FAIL %s:%d: %s\n",__FILE__,__LINE__,#x);return 1;}}while(0)
struct file_drive { FILE *f; };
static uint32_t readsector(void *context,uint32_t mode,uint32_t fn,
                           uint32_t lbn,uint32_t bytes,void *buffer){
    struct file_drive *d=context;
    if(mode || fn!=1u || bytes!=512u || !buffer ||
       fseek(d->f,(long)lbn*512L,SEEK_SET) ||
       fread(buffer,1,512u,d->f)!=512u)return BOO_IO_FAILED;
    return BOO_IO_NORMAL;
}
static void word(uint8_t *p,uint16_t x){p[0]=(uint8_t)x;p[1]=(uint8_t)(x>>8);}
static void dword(uint8_t *p,uint32_t x){
    for(unsigned i=0;i<4u;i++)p[i]=(uint8_t)(x>>(8u*i));
}
int main(int argc,char **argv){
    CHECK(argc==2);
    struct file_drive d={fopen(argv[1],"rb")};CHECK(d.f);
    const struct boo_io_driver disk={readsector,&d,0u};
    struct port_f11_volume volume={0};
    CHECK(port_f11_mount(&disk,1u,&volume)==PORT_F11_HOME_OK);
    struct port_f11_fid fid={0};uint8_t header[512];
    uint32_t first_lbn=0u,allocated_blocks=0u;
    CHECK(port_f11_open_named_file(&disk,1u,&volume,"[SYSEXE]SYS.EXE",
       &fid,header,&first_lbn,&allocated_blocks)==PORT_F11_HOME_OK);
    CHECK(fid.number==11u&&first_lbn>0u&&allocated_blocks>0u);
    struct port_f11_extent extents[48]={0};uint32_t count=0u;
    CHECK(port_f11_collect_extents(header,extents,48u,&count)==PORT_F11_HOME_OK);
    CHECK(count>0u&&count<48u);
    uint8_t normalized[1024];memset(normalized,0xa5,sizeof normalized);
    dword(normalized,8u*count);
    uint32_t total=0;
    for(uint32_t i=0;i<count;i++){
        CHECK(extents[i].blocks && extents[i].blocks<=UINT16_MAX);
        CHECK(extents[i].lbn+extents[i].blocks>extents[i].lbn);
        dword(normalized+4u+i*8u,extents[i].blocks);
        dword(normalized+8u+i*8u,extents[i].lbn);
        total+=extents[i].blocks;
    }
    CHECK(total>=allocated_blocks);
    uint8_t templ[48];memset(templ,0x6a,sizeof templ);
    const struct port_ini_syswcb_layout layout={48u,4u,6u};
    word(templ+layout.size_word_offset,48u);
    struct port_ini_syswcb out={0};
    CHECK(port_ini_syswcb_from_boot_map(normalized,sizeof normalized,
       templ,sizeof templ,&layout,&out)==PORT_INI_SYSWCB_OK);
    CHECK(out.mapping_count==count&&out.record_length==48u+6u*count);
    CHECK(out.record_size_field==48u+6u*count);
    for(uint32_t i=0;i<count;i++){
        uint16_t blocks=0u;uint32_t lbn=0u;
        CHECK(port_ini_syswcb_retrieval(&out,i,&blocks,&lbn)==PORT_INI_SYSWCB_OK);
        CHECK(blocks==extents[i].blocks&&lbn==extents[i].lbn);
    }
    CHECK(out.retrieval_pointers[out.mapping_count*6u]==0xa5u);
    fclose(d.f);
    puts("Real Files-11 SYS.EXE INDEXF extent converted to native INIT WCB record PASS");
    return 0;
}
