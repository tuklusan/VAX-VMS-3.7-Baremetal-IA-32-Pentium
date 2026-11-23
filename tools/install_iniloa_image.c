/* Files-11 ODS-2 installation of the separately linked native INILOA.EXE.
 * SYS.EXE already occupies FID 11. Extend INDEXF.SYS with a full cluster
 * for the next file header rather than overwriting BITMAP.SYS at LBN 24.
 * Allocation and file lookup go through the source-derived ODS-2 routines. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "vms_boot_files11.h"
#include "vms_f11_home.h"
#define BLOCK 512u
#define DISK_BLOCKS 8064u
#define INI_FID 12u
struct image_disk { FILE *f; };
static uint16_t r16(const uint8_t *b){return (uint16_t)(b[0]|((uint16_t)b[1]<<8));}
static void w16(uint8_t *b,uint16_t v){b[0]=(uint8_t)v;b[1]=(uint8_t)(v>>8);}
static void w32(uint8_t *b,uint32_t v){w16(b,(uint16_t)v);w16(b+2,(uint16_t)(v>>16));}
static int io(struct image_disk *d,uint32_t lbn,uint8_t b[BLOCK],int wr){
    if(lbn>=DISK_BLOCKS||fseek(d->f,(long)(lbn*BLOCK),SEEK_SET))return 0;
    return (wr?fwrite(b,1,BLOCK,d->f):fread(b,1,BLOCK,d->f))==BLOCK;
}
static int put(struct image_disk *d,uint32_t lbn,const uint8_t b[BLOCK]){
    uint8_t tmp[BLOCK];memcpy(tmp,b,BLOCK);return io(d,lbn,tmp,1);
}
static uint32_t disk_read(void *ctx,uint32_t mode,uint32_t fn,
                           uint32_t lbn,uint32_t bytes,void *dest){
    return !mode&&fn==1u&&bytes==BLOCK&&dest&&
        io(ctx,lbn,dest,0)?BOO_IO_NORMAL:BOO_IO_FAILED;
}
static int free_clusters(struct image_disk *d,uint32_t start,
                          uint32_t blocks,uint32_t cluster){
    if(!blocks||!cluster||start%cluster||start+blocks<start||
       start+blocks>DISK_BLOCKS)return 0;
    uint8_t bitmap[BLOCK];if(!io(d,25u,bitmap,0))return 0;
    uint32_t count=(blocks+cluster-1u)/cluster;
    for(uint32_t i=0;i<count;i++){
        uint32_t bit=start/cluster+i;
        if(bit>=4096u||!(bitmap[bit/8u]&(1u<<(bit%8u))))return 0;
    }
    return 1;
}
static int reserve_clusters(struct image_disk *d,uint32_t start,
                             uint32_t blocks,uint32_t cluster){
    uint8_t bitmap[BLOCK];if(!free_clusters(d,start,blocks,cluster)||
                             !io(d,25u,bitmap,0))return 0;
    for(uint32_t i=0;i<(blocks+cluster-1u)/cluster;i++){
        uint32_t bit=start/cluster+i;
        bitmap[bit/8u]&=(uint8_t)~(1u<<(bit%8u));
    }
    return put(d,25u,bitmap);
}
static int extend_index(struct image_disk *d,uint32_t index_lbn){
    uint8_t b[BLOCK];
    /* The original FID 1 INDEXF.SYS maps its initial 24 blocks at LBN0.
     * FID 12 maps VBN 25, which must be placed in new storage. */
    if(!io(d,13u,b,0)||r16(b+8u)!=1u||r16(b+10u)!=1u||
       b[58u]!=2u||r16(b+130u)!=0x4017u||r16(b+132u)!=0u||
       b[134u]!=0u||b[135u]!=0u)return 0;
    w16(b+134u,0x4002u);w16(b+136u,(uint16_t)index_lbn);
    b[58u]=4u;
    w32(b+24u,27u<<16u);w32(b+28u,26u<<16u);
    port_f11_checksum2(b,510u);
    return put(d,13u,b);
}
static int add_header(struct image_disk *d,uint32_t header_lbn,
                       uint32_t data_lbn,uint32_t blocks,uint32_t bytes){
    uint8_t b[BLOCK];
    if(!io(d,16u,b,0)||b[7u]!=2u||b[1u]!=65u||
       r16(b+8u)!=4u||r16(b+10u)!=4u)return 0;
    w16(b+8u,INI_FID);w16(b+10u,INI_FID);
    b[20u]=1u;b[21u]=0u;
    w32(b+24u,blocks<<16u);w32(b+28u,(blocks+1u)<<16u);
    w16(b+32u,(uint16_t)(bytes%BLOCK));
    memset(b+76u,' ',20u);memcpy(b+76u,"INILOA.EXE;1",12u);
    memset(b+130u,0,380u);
    if(blocks<=256u && data_lbn<(1u<<22u)){
        w16(b+130u,(uint16_t)(0x4000u|((data_lbn>>16u)&63u)<<8u|(blocks-1u)));
        w16(b+132u,(uint16_t)data_lbn);b[58u]=2u;
    }else if(blocks<=16384u){
        w16(b+130u,(uint16_t)(0x8000u|(blocks-1u)));
        w32(b+132u,data_lbn);b[58u]=3u;
    }else return 0;
    port_f11_checksum2(b,510u);
    return put(d,header_lbn,b);
}
static int add_directory(struct image_disk *d){
    uint8_t b[BLOCK];
    /* Keep the existing SYS.EXE entry; insert INILOA.EXE before it. */
    if(!io(d,30u,b,0)||r16(b)!=20u||b[5u]!=7u||
       memcmp(b+6u,"SYS.EXE",7u)||r16(b+16u)!=11u||
       r16(b+18u)!=11u||r16(b+22u)!=0xffffu)return 0;
    memmove(b+24u,b,24u);
    memset(b,0,24u);
    w16(b,22u);w16(b+2u,1u);b[5u]=10u;
    memcpy(b+6u,"INILOA.EXE",10u);
    w16(b+16u,1u);w16(b+18u,INI_FID);
    w16(b+20u,INI_FID);w16(b+22u,0u);
    return put(d,30u,b);
}
static int set_index_bitmap(struct image_disk *d){
    uint8_t b[BLOCK];
    if(!io(d,12u,b,0)||r16(b)!=0x07ffu)return 0;
    w16(b,0x0fffu);return put(d,12u,b);
}
static int verify(struct image_disk *d,FILE *src,uint32_t bytes,
                  uint32_t header_lbn,uint32_t data_lbn){
    const struct boo_io_driver driver={disk_read,d,0u};
    struct port_f11_volume vol;struct port_f11_fid id;
    uint8_t h[BLOCK];uint32_t first=0,blocks=0;
    if(port_f11_mount(&driver,1u,&vol)!=PORT_F11_HOME_OK||
       port_f11_open_named_file(&driver,1u,&vol,"[SYSEXE]INILOA.EXE",
          &id,h,&first,&blocks)!=PORT_F11_HOME_OK||
       id.number!=INI_FID||id.sequence!=INI_FID||first!=data_lbn||
       blocks!=(bytes+511u)/512u||
       port_f11_read_file_header(&driver,1u,&vol,id,h)!=PORT_F11_HOME_OK||
       !header_lbn||fseek(src,0,SEEK_SET))return 0;
    for(uint32_t pos=0;pos<bytes;){
        uint8_t diskblock[BLOCK],source[BLOCK];
        uint32_t n=bytes-pos;if(n>BLOCK)n=BLOCK;
        if(port_f11_read_vbn(&driver,1u,h,pos/BLOCK+1u,diskblock)!=PORT_F11_HOME_OK||
           fread(source,1,n,src)!=n||memcmp(source,diskblock,n))return 0;
        pos+=n;
    }
    return 1;
}
int main(int argc,char **argv){
    if(argc!=3){fprintf(stderr,"usage: %s portdisk.img iniloa.exe\n",argv[0]);return 2;}
    struct image_disk d={fopen(argv[1],"r+b")};FILE *src=fopen(argv[2],"rb");
    if(!d.f||!src){perror("open");if(d.f)fclose(d.f);if(src)fclose(src);return 2;}
    int ok=0;
    do{
        if(fseek(src,0,SEEK_END))break;
        long n=ftell(src);if(n<512L||n>128L*BLOCK)break;
        uint32_t bytes=(uint32_t)n,blocks=(bytes+511u)/512u;
        const struct boo_io_driver driver={disk_read,&d,0u};
        struct port_f11_volume vol;struct port_f11_fid sys,id;
        uint8_t h[BLOCK];uint32_t start=0,sysblocks=0;
        if(port_f11_mount(&driver,1u,&vol)!=PORT_F11_HOME_OK||
           vol.home.cluster!=3u||vol.home.index_bitmap_lbn!=12u||
           vol.home.max_files<INI_FID||
           port_f11_open_named_file(&driver,1u,&vol,"[SYSEXE]SYS.EXE",
                &sys,h,&start,&sysblocks)!=PORT_F11_HOME_OK||
           sys.number!=11u||sys.sequence!=11u||start!=33u||
           port_f11_open_named_file(&driver,1u,&vol,"[SYSEXE]INILOA.EXE",
                &id,h,&start,&sysblocks)!=PORT_F11_FILE_NOT_FOUND)break;
        /* Restore SYS size after the missing-INILOA lookup. */
        if(port_f11_open_named_file(&driver,1u,&vol,"[SYSEXE]SYS.EXE",
                 &sys,h,&start,&sysblocks)!=PORT_F11_HOME_OK)break;
        uint32_t index_start=(start+sysblocks+2u)/3u*3u;
        uint32_t data_start=index_start+3u;
        if(index_start>65535u||!free_clusters(&d,index_start,3u,3u)||
           !free_clusters(&d,data_start,blocks,3u))break;
        if(fseek(src,0,SEEK_SET))break;
        int written=1;
        for(uint32_t i=0;i<blocks;i++){
            uint8_t b[BLOCK];uint32_t count=bytes-i*BLOCK;
            if(count>BLOCK)count=BLOCK;
            if(fread(b,1,count,src)!=count){written=0;break;}
            if(count<BLOCK)memset(b+count,0,BLOCK-count);
            if(!put(&d,data_start+i,b)){written=0;break;}
        }
        if(!written||ferror(src))break;
        if(!add_header(&d,index_start,data_start,blocks,bytes)||
           !extend_index(&d,index_start)||!set_index_bitmap(&d)||
           !add_directory(&d)||
           !reserve_clusters(&d,index_start,3u,3u)||
           !reserve_clusters(&d,data_start,blocks,3u)||
           fflush(d.f)||!verify(&d,src,bytes,index_start,data_start))break;
        printf("Installed INILOA.EXE: FID %u, header LBN %u, data LBN %u, %u blocks, %u bytes; readback verified\n",
                 INI_FID,index_start,data_start,blocks,bytes);
        ok=1;
    }while(0);
    if(!ok)fputs("INILOA Files-11 installation/verification failed\n",stderr);
    fclose(src);fclose(d.f);return ok?0:1;
}
