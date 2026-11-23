#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "vms_f11_chain.h"
#define CHECK(x) do{if(!(x)){fprintf(stderr,"FIL$RDCHKFILHDR extension %d: %s\n",__LINE__,#x);return 1;}}while(0)
static void put16(uint8_t *p,unsigned at,uint16_t x){p[at]=(uint8_t)x;p[at+1u]=(uint8_t)(x>>8u);}
static void sum(uint8_t *b){uint16_t x=0u;for(unsigned i=0;i<510u;i+=2u)x=(uint16_t)(x+port_dq_index_word(b,i));put16(b,510u,x);}
struct fake {uint8_t first[512],ext[512];uint32_t first_lbn,ext_lbn;unsigned reads,fail_at;};
static uint32_t readsec(void *o,uint32_t lbn,uint8_t out[512]){
    struct fake *f=o;
    if(++f->reads==f->fail_at)return PORT_DQ_DIR_MAP;
    if(lbn==f->first_lbn){memcpy(out,f->first,512u);return PORT_DQ_DIR_OK;}
    if(lbn==f->ext_lbn){memcpy(out,f->ext,512u);return PORT_DQ_DIR_OK;}
    if(lbn==700u||lbn==1234u){for(unsigned k=0;k<512u;k++)out[k]=(uint8_t)(lbn+k);return PORT_DQ_DIR_OK;}
    return PORT_DQ_DIR_MAP;
}
static int diskread(FILE *d,uint32_t n,uint8_t b[512]){
    return !fseek(d,(long)n*512L,SEEK_SET)&&fread(b,1,512u,d)==512u;
}
int main(void){
    FILE *disk=fopen("portdisk.img","rb");CHECK(disk);
    uint8_t home[512],index[512],mfd[512];uint32_t lbn=0u,vbn=0u;
    CHECK(diskread(disk,1u,home));
    CHECK(port_dq_index_location(home,&lbn)==PORT_DQ_INDEX_OK);
    CHECK(diskread(disk,lbn,index));
    CHECK(port_dq_mfd_location(home,index,&vbn,&lbn)==PORT_DQ_MFD_OK);
    CHECK(diskread(disk,lbn,mfd));
    struct fake f={0};
    const struct port_dq_dir_fid fid1={11u,11u,0u},fid2={14u,14u,0u};
    CHECK(port_dq_image_header_lbn(home,index,&fid1,&f.first_lbn)==PORT_DQ_IMAGE_OK);
    CHECK(port_dq_image_header_lbn(home,index,&fid2,&f.ext_lbn)==PORT_DQ_IMAGE_OK);
    CHECK(f.first_lbn!=f.ext_lbn);
    CHECK(diskread(disk,f.first_lbn,f.first));
    memcpy(f.ext,f.first,512u);
    put16(f.first,14u,14u);put16(f.first,16u,14u);put16(f.first,18u,0u);
    put16(f.ext,8u,14u);put16(f.ext,10u,14u);put16(f.ext,12u,0u);
    put16(f.ext,14u,0u);put16(f.ext,16u,0u);put16(f.ext,18u,0u);
    const unsigned a=(unsigned)f.first[1u]*2u;
    CHECK(a+8u<510u);
    f.first[58u]=2u;f.ext[58u]=2u;
    put16(f.first,a,UINT16_C(0x4000));put16(f.first,a+2u,700u);
    put16(f.ext,a,UINT16_C(0x4000));put16(f.ext,a+2u,1234u);
    sum(f.first);sum(f.ext);
    const struct port_f11_boot_volume v={home,index,mfd,readsec,&f,1u};
    struct port_f11_chain chain={0};
    CHECK(port_f11_chain_collect(&v,&fid1,&chain)==PORT_F11_CHAIN_OK);
    CHECK(f.reads==2u&&chain.headers==2u&&chain.blocks==2u&&chain.count==2u);
    CHECK(port_f11_chain_lbn(&chain,1u,&lbn)==PORT_F11_CHAIN_OK&&lbn==700u);
    CHECK(port_f11_chain_lbn(&chain,2u,&lbn)==PORT_F11_CHAIN_OK&&lbn==1234u);
    CHECK(port_f11_chain_lbn(&chain,3u,&lbn)==PORT_F11_CHAIN_MAP);
    uint8_t received[700];uint32_t done=UINT32_MAX;
    CHECK(port_f11_chain_read(&v,&chain,400u,700u,received,
         sizeof received,&done)==PORT_F11_CHAIN_MAP); /* exceeds two blocks */
    CHECK(done==UINT32_MAX);
    CHECK(port_f11_chain_read(&v,&chain,400u,624u,received,
         sizeof received,&done)==PORT_F11_CHAIN_OK);
    CHECK(done==624u);
    for(unsigned i=0;i<624u;i++){
        uint32_t address=i+400u;
        const uint32_t at=address<512u?700u:1234u;
        CHECK(received[i]==(uint8_t)(at+(address%512u)));
    }
    put16(f.ext,14u,11u);put16(f.ext,16u,11u);sum(f.ext);
    CHECK(port_f11_chain_collect(&v,&fid1,&chain)==PORT_F11_CHAIN_CYCLE);
    put16(f.ext,14u,0u);put16(f.ext,16u,0u);sum(f.ext);
    f.fail_at=f.reads+2u;
    CHECK(port_f11_chain_collect(&v,&fid1,&chain)==PORT_F11_CHAIN_IO);
    f.fail_at=0u;
    CHECK(port_f11_chain_collect(&v,&fid1,&chain)==PORT_F11_CHAIN_OK);
    CHECK(!fclose(disk));
    puts("FIL$RDCHKFILHDR synthetic extension FID, disjoint extents, loop and I/O rollback PASS");
    return 0;
}
