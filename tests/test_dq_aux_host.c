#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "vms_dq_aux_images.h"
#define CHECK(x) do{if(!(x)){fprintf(stderr,"aux FH2/image %d: %s\n",__LINE__,#x);return 1;}}while(0)
static int readblk(FILE *f,uint32_t l,uint8_t *b){return fseek(f,(long)l*512L,SEEK_SET)==0&&fread(b,1,512u,f)==512u;}
static void set32(uint8_t*b,unsigned p,uint32_t n){for(unsigned i=0;i<4;i++)b[p+i]=(uint8_t)(n>>(8*i));}
static void set16(uint8_t*b,unsigned p,uint16_t n){b[p]=(uint8_t)n;b[p+1]=(uint8_t)(n>>8);}
static int one(FILE *disk,const uint8_t home[512],const uint8_t index[512],
               const uint8_t dir[512],const char *name,
               const char *native_path,uint32_t start,uint32_t end){
    struct port_dq_dir_fid fid={0};
    uint8_t header[512],elf[512],sector[512],native[512],mut[512];
    CHECK(port_dq_dir_find(dir,name,strlen(name),1u,&fid)==PORT_DQ_DIR_OK);
    uint32_t lbn=0;
    CHECK(port_dq_image_header_lbn(home,index,&fid,&lbn)==PORT_DQ_IMAGE_OK);
    CHECK(readblk(disk,lbn,header));
    CHECK(port_dq_image_verify_fh2(header,&fid)==PORT_DQ_IMAGE_OK);
    CHECK(port_dq_image_first_data(header,&lbn)==PORT_DQ_IMAGE_OK);
    CHECK(readblk(disk,lbn,elf));
    struct port_dq_aux_segment seg={0};
    CHECK(port_dq_aux_descriptor(elf,start,end,&seg)==PORT_DQ_AUX_OK);
    CHECK(seg.va==start&&seg.sectors>=2u);
    FILE *f=fopen(native_path,"rb");CHECK(f);
    unsigned total=0;
    for(uint32_t n=0;n<seg.sectors;n++){
        uint32_t disk_lbn=0,va=0,bytes=0;
        CHECK(port_dq_aux_sector(header,&seg,n,&disk_lbn,&va,&bytes)==PORT_DQ_AUX_OK);
        CHECK(va==seg.va+512u*n&&bytes>=1u&&bytes<=512u);
        CHECK(readblk(disk,disk_lbn,sector));
        CHECK(fseek(f,(long)seg.offset+(long)n*512L,SEEK_SET)==0);
        CHECK(fread(native,1,bytes,f)==bytes);
        CHECK(memcmp(sector,native,bytes)==0);
        total+=bytes;
    }
    CHECK(total==seg.filesz);
    for(uint32_t n=0;n<10u;n++){
        uint32_t a=0xdeadbeefu,b=0xdeadbeefu,c=0xdeadbeefu;
        CHECK(port_dq_aux_sector(header,&seg,UINT32_MAX-n,&a,&b,&c)!=PORT_DQ_AUX_OK);
        CHECK(a==0xdeadbeefu&&b==0xdeadbeefu&&c==0xdeadbeefu);
    }
    for(unsigned n=0;n<16u;n++){
        memcpy(mut,elf,512u);
        switch(n){
        case 0:mut[0]=0;break;case 1:mut[4]=2;break;
        case 2:set16(mut,16u,3u);break;case 3:set16(mut,18u,40u);break;
        case 4:set32(mut,28u,500u);break;case 5:set16(mut,44u,0u);break;
        case 6:set16(mut,44u,11u);break;case 7:set32(mut,24u,end);break;
        case 8:set32(mut,52u,0u);break;case 9:set32(mut,52u+4u,4097u);break;
        case 10:set32(mut,52u+8u,end);break;case 11:set32(mut,52u+12u,0u);break;
        case 12:set32(mut,52u+16u,0u);break;case 13:set32(mut,52u+20u,0u);break;
        case 14:set32(mut,52u+24u,7u);break;
        default:set32(mut,52u+16u,UINT32_MAX);break;
        }
        struct port_dq_aux_segment keep={0};keep.va=0xdeadbeefu;
        CHECK(port_dq_aux_descriptor(mut,start,end,&keep)!=PORT_DQ_AUX_OK);
        CHECK(keep.va==0xdeadbeefu);
    }
    CHECK(fclose(f)==0);
    return 0;
}
int main(void){
    FILE *f=fopen("portdisk.img","rb");CHECK(f);
    uint8_t home[512],index[512],mfd[512],d[512],s[512],sysexe[512];
    CHECK(readblk(f,1u,home));
    uint32_t index_lbn=0;
    CHECK(port_dq_index_location(home,&index_lbn)==PORT_DQ_INDEX_OK);
    CHECK(readblk(f,index_lbn,index));
    uint32_t vbn=0,lbn=0;
    CHECK(port_dq_mfd_location(home,index,&vbn,&lbn)==PORT_DQ_MFD_OK);
    CHECK(readblk(f,lbn,mfd));
    CHECK(port_dq_dir_map_vbn(mfd,1u,&lbn)==PORT_DQ_DIR_OK);
    CHECK(readblk(f,lbn,d));
    struct port_dq_dir_fid fid;
    CHECK(port_dq_dir_find(d,"SYSEXE.DIR",10u,1u,&fid)==PORT_DQ_DIR_OK);
    CHECK(port_dq_image_header_lbn(home,index,&fid,&lbn)==PORT_DQ_IMAGE_OK);
    CHECK(readblk(f,lbn,s));
    CHECK(port_dq_dir_map_vbn(s,1u,&lbn)==PORT_DQ_DIR_OK);
    CHECK(readblk(f,lbn,sysexe));
    CHECK(one(f,home,index,sysexe,"SYSLOA386.EXE","build/sysloa386.exe",
              0x003e0000u,0x003f0000u)==0);
    CHECK(one(f,home,index,sysexe,"INILOA.EXE","build/iniloa.exe",
              0x00380000u,0x003e0000u)==0);
    CHECK(port_dq_aux_descriptor(NULL,0x00380000u,0x003e0000u,NULL)==PORT_DQ_AUX_BAD);
    CHECK(fclose(f)==0);
    puts("Source FIL$FINDFILID/FIL$READVBN verifies both real native SYSLOA386 and INILOA executable images PASS");
    return 0;
}
