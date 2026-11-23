#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "vms_dq_imageprobe.h"
#define CHECK(x) do{if(!(x)){fprintf(stderr,"multi VBN directory line %d: %s\n",__LINE__,#x);return 1;}}while(0)
struct diskctx{uint8_t sectors[3][512];uint32_t physical[3];unsigned calls;unsigned fail_at;};
static uint32_t fake_read(void *ctx,uint32_t lbn,uint8_t out[512]){
    struct diskctx *d=ctx;d->calls++;
    if(d->fail_at==d->calls)return PORT_DQ_DIR_MAP;
    for(unsigned n=0;n<3u;n++)if(d->physical[n]==lbn){memcpy(out,d->sectors[n],512u);return PORT_DQ_DIR_OK;}
    return PORT_DQ_DIR_MAP;
}
static int read_sector(FILE *f,uint32_t lbn,uint8_t *b){return fseek(f,512L*(long)lbn,SEEK_SET)==0&&fread(b,1,512,f)==512;}
static void put16(uint8_t*b,unsigned i,uint16_t v){b[i]=(uint8_t)v;b[i+1]=(uint8_t)(v>>8u);}
static void checksum(uint8_t b[512]){uint16_t sum=0;for(unsigned n=0;n<510u;n+=2)sum=(uint16_t)(sum+port_dq_index_word(b,n));put16(b,510,sum);}
int main(void){
    FILE *disk=fopen("portdisk.img","rb");CHECK(disk);
    uint8_t home[512],index[512],mfd[512],dir[512],hdr[512];
    uint32_t lbn,vbn;CHECK(read_sector(disk,1u,home));
    CHECK(port_dq_index_location(home,&lbn)==PORT_DQ_INDEX_OK);
    CHECK(read_sector(disk,lbn,index));
    CHECK(port_dq_mfd_location(home,index,&vbn,&lbn)==PORT_DQ_MFD_OK);
    CHECK(read_sector(disk,lbn,mfd));
    CHECK(port_dq_dir_map_vbn(mfd,1u,&lbn)==PORT_DQ_DIR_OK);
    CHECK(read_sector(disk,lbn,dir));
    struct port_dq_dir_fid sysexe={0};
    CHECK(port_dq_dir_find(dir,"SYSEXE.DIR",10u,1u,&sysexe)==PORT_DQ_DIR_OK);
    CHECK(port_dq_image_header_lbn(home,index,&sysexe,&lbn)==PORT_DQ_IMAGE_OK);
    CHECK(read_sector(disk,lbn,hdr));
    CHECK(port_dq_dir_map_vbn(hdr,1u,&lbn)==PORT_DQ_DIR_OK);
    struct diskctx d={0};d.physical[0]=lbn;
    CHECK(read_sector(disk,lbn,d.sectors[0]));
    struct port_dq_dir_fid fid={0};uint32_t found=0;
    CHECK(port_dq_dir_find_blocks(hdr,"INILOA.EXE",10u,1u,1u,
          fake_read,&d,&fid,&found)==PORT_DQ_DIR_OK);
    CHECK(fid.number==12u&&found==1u&&d.calls==1u);
    /* A valid ODS-2 FH2 fragmented extent has its second VBN mapped to
     * a non-adjacent disk sector. Generate a two-record directory: first
     * VBN valid but no match, second has the real INILOA entry. */
    memcpy(d.sectors[1],d.sectors[0],512u);
    memcpy(d.sectors[2],d.sectors[0],512u);
    /* Reuse real directory bytes but ask for a name that is present
     * only on VBN2 after masking VBN1's contents with end marker. */
    put16(d.sectors[0],0u,UINT16_MAX);
    d.physical[0]=60u;d.physical[1]=61u;d.physical[2]=160u;
    memcpy(d.sectors[1],d.sectors[2],512u);
    /* Copy a valid source FH2 and replace retrieval descriptor with two
     * extents; the map uses 16-bit count/LBN words, not disk adjacency. */
    uint8_t fakefh2[512];memcpy(fakefh2,hdr,512u);
    const unsigned map_offset=(unsigned)fakefh2[1u]*2u;
    CHECK(map_offset<500u);
    fakefh2[58u]=4u;
    put16(fakefh2,map_offset,UINT16_C(0x4000));
    put16(fakefh2,map_offset+2u,60u);
    put16(fakefh2,map_offset+4u,UINT16_C(0x4000));
    put16(fakefh2,map_offset+6u,160u);
    checksum(fakefh2);
    d.calls=0u;
    CHECK(port_dq_dir_find_blocks(fakefh2,"INILOA.EXE",10u,1u,2u,
          fake_read,&d,&fid,&found)==PORT_DQ_DIR_OK);
    CHECK(found==2u&&fid.number==12u&&d.calls==2u);
    for(unsigned fail=1u;fail<=2u;fail++){
        d.calls=0;d.fail_at=fail;fid.number=0x7fffu;found=0xdeadbeefu;
        CHECK(port_dq_dir_find_blocks(fakefh2,"INILOA.EXE",10u,1u,2u,
             fake_read,&d,&fid,&found)==PORT_DQ_DIR_MAP);
        CHECK(fid.number==0x7fffu&&found==0xdeadbeefu);
    }
    d.fail_at=0;d.calls=0;
    CHECK(port_dq_dir_find_blocks(fakefh2,"NOT.EXE",7u,1u,2u,
             fake_read,&d,&fid,&found)==PORT_DQ_DIR_NOT_FOUND);
    CHECK(port_dq_dir_find_blocks(fakefh2,"INILOA.EXE",10u,1u,0u,
             fake_read,&d,&fid,&found)==PORT_DQ_DIR_BAD);
    CHECK(port_dq_dir_find_blocks(fakefh2,"INILOA.EXE",10u,1u,UINT32_MAX,
             fake_read,&d,&fid,&found)==PORT_DQ_DIR_BAD);
    CHECK(port_dq_dir_find_blocks(fakefh2,"INILOA.EXE",10u,1u,2u,
             NULL,&d,&fid,&found)==PORT_DQ_DIR_BAD);
    CHECK(fclose(disk)==0);
    puts("Source FIL$FINDFILID fragmented multi-VBN Files-11 directory traversal PASS");
    return 0;
}
