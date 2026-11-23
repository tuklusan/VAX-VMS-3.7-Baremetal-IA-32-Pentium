/* Files-11 ODS-2 SYSLOA386.EXE installation. SYS and INILOA already use
 * FIDs 11 and 12. INDEXF's allocated three-block extension has room for
 * FID13 at its next mapped VBN; never overwrite the bitmap or other files.
 * Original SYSLOAxxx boot order is preserved for the native PC CPU type. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "vms_boot_files11.h"
#include "vms_f11_home.h"
#define BLOCK 512u
#define DISK_BLOCKS 8064u
#define SYSLOA_FID 13u
struct disk {FILE *f;};
static uint16_t u16(const uint8_t *p){return (uint16_t)(p[0]|((uint16_t)p[1]<<8));}
static void p16(uint8_t *p,uint16_t v){p[0]=(uint8_t)v;p[1]=(uint8_t)(v>>8);}
static void p32(uint8_t *p,uint32_t v){p16(p,(uint16_t)v);p16(p+2,(uint16_t)(v>>16));}
static int sector(struct disk *d,uint32_t lbn,uint8_t b[BLOCK],int write){
    if(!d||!d->f||lbn>=DISK_BLOCKS||fseek(d->f,(long)lbn*BLOCK,SEEK_SET))return 0;
    return (write?fwrite(b,1,BLOCK,d->f):fread(b,1,BLOCK,d->f))==BLOCK;
}
static int put(struct disk *d,uint32_t lbn,const uint8_t *src){
    uint8_t b[BLOCK];memcpy(b,src,BLOCK);return sector(d,lbn,b,1);
}
static uint32_t diskread(void *ctx,uint32_t mode,uint32_t fn,
                         uint32_t lbn,uint32_t count,void *dest){
    return !mode&&fn==1u&&count==BLOCK&&dest&&sector(ctx,lbn,dest,0)?
        BOO_IO_NORMAL:BOO_IO_FAILED;
}
static int free_clusters(struct disk *d,uint32_t at,uint32_t blocks,uint32_t cf){
    uint8_t b[BLOCK];
    if(!blocks||!cf||at%cf||at>=DISK_BLOCKS||blocks>DISK_BLOCKS-at||
       !sector(d,25u,b,0))return 0;
    for(uint32_t j=0;j<(blocks+cf-1u)/cf;j++){
        uint32_t bit=at/cf+j;
        if(bit>=4096u||!(b[bit/8u]&(1u<<(bit%8u))))return 0;
    }
    return 1;
}
static int mark_clusters(struct disk *d,uint32_t at,uint32_t blocks,uint32_t cf){
    uint8_t b[BLOCK];
    if(!free_clusters(d,at,blocks,cf)||!sector(d,25u,b,0))return 0;
    for(uint32_t j=0;j<(blocks+cf-1u)/cf;j++){
        uint32_t bit=at/cf+j;
        b[bit/8u]&=(uint8_t)~(1u<<(bit%8u));
    }
    return put(d,25u,b);
}
static int add_header(struct disk *d,uint32_t header_lbn,
                      const uint8_t template_header[BLOCK],
                      uint32_t data_lbn,uint32_t blocks,uint32_t bytes){
    uint8_t b[BLOCK];memcpy(b,template_header,BLOCK);
    if(b[7]!=2u||b[1]!=65u||!blocks||blocks>256u||data_lbn>65535u)return 0;
    p16(b+8u,SYSLOA_FID);p16(b+10u,SYSLOA_FID);
    b[20u]=1u;b[21u]=0u;
    p32(b+24u,blocks<<16u);p32(b+28u,(blocks+1u)<<16u);
    p16(b+32u,(uint16_t)(bytes%BLOCK));
    memset(b+76u,' ',20u);memcpy(b+76u,"SYSLOA386.EXE;1",15u);
    memset(b+130u,0,380u);
    p16(b+130u,(uint16_t)(0x4000u|((blocks-1u)&0xffu)));
    p16(b+132u,(uint16_t)data_lbn);b[58u]=2u;
    port_f11_checksum2(b,510u);
    return put(d,header_lbn,b);
}
static int add_directory(struct disk *d){
    uint8_t b[BLOCK];
    if(!sector(d,30u,b,0))return 0;
    unsigned at=0u,seen=0u;
    while(at+2u<=BLOCK){
        uint16_t n=u16(b+at);
        if(n==0xffffu)break;
        if(n<12u || at+2u+n>BLOCK)return 0;
        if(b[at+5u]==10u && !memcmp(b+at+6u,"INILOA.EXE",10u))seen|=1u;
        if(b[at+5u]==7u && !memcmp(b+at+6u,"SYS.EXE",7u))seen|=2u;
        at+=(unsigned)n+2u;
    }
    if(seen!=3u || at+28u+2u>BLOCK || u16(b+at)!=0xffffu)return 0;
    memset(b+at,0,30u);
    p16(b+at,26u);p16(b+at+2u,1u);
    b[at+5u]=13u;memcpy(b+at+6u,"SYSLOA386.EXE",13u);
    const unsigned v=at+6u+14u;
    p16(b+v,1u);p16(b+v+2u,SYSLOA_FID);
    p16(b+v+4u,SYSLOA_FID);p16(b+v+6u,0u);
    p16(b+at+28u,0xffffu);
    return put(d,30u,b);
}
static int mark_index_file(struct disk *d){
    uint8_t b[BLOCK];if(!sector(d,12u,b,0)||u16(b)!=0x0fffu)return 0;
    p16(b,0x1fffu);return put(d,12u,b);
}
static int verify(struct disk *d,FILE *src,uint32_t bytes,
                  uint32_t head_lbn,uint32_t data_lbn){
    const struct boo_io_driver io={diskread,d,0u};
    struct port_f11_volume volume;struct port_f11_fid fid;
    uint8_t header[BLOCK];uint32_t first=0,blocks=0;
    if(port_f11_mount(&io,1u,&volume)!=PORT_F11_HOME_OK ||
       port_f11_open_named_file(&io,1u,&volume,"[SYSEXE]SYSLOA386.EXE",
               &fid,header,&first,&blocks)!=PORT_F11_HOME_OK ||
       fid.number!=SYSLOA_FID||fid.sequence!=SYSLOA_FID||
       first!=data_lbn||blocks!=(bytes+BLOCK-1u)/BLOCK||
       port_f11_read_file_header(&io,1u,&volume,fid,header)!=PORT_F11_HOME_OK||
       fseek(src,0L,SEEK_SET))return 0;
    uint32_t header_actual=0;
    if(port_f11_vbn_to_lbn(volume.index_header,
               volume.index_vbn_offset+SYSLOA_FID,&header_actual)!=PORT_F11_HOME_OK||
       header_actual!=head_lbn)return 0;
    for(uint32_t at=0;at<bytes;){
        uint32_t n=bytes-at;if(n>BLOCK)n=BLOCK;
        uint8_t block[BLOCK],original[BLOCK];
        if(port_f11_read_vbn(&io,1u,header,at/BLOCK+1u,block)!=PORT_F11_HOME_OK||
           fread(original,1,n,src)!=n||memcmp(block,original,n))return 0;
        at+=n;
    }
    return 1;
}
int main(int argc,char **argv){
    if(argc!=3){fprintf(stderr,"usage: %s portdisk.img sysloa386.exe\n",argv[0]);return 2;}
    struct disk disk={fopen(argv[1],"r+b")};FILE *image=fopen(argv[2],"rb");
    if(!disk.f||!image){perror("open");if(disk.f)fclose(disk.f);if(image)fclose(image);return 2;}
    int ok=0;
    do{
        if(fseek(image,0,SEEK_END))break;
        long length=ftell(image);if(length<512L||length>65535L)break;
        uint32_t bytes=(uint32_t)length,blocks=(bytes+BLOCK-1u)/BLOCK;
        const struct boo_io_driver io={diskread,&disk,0u};
        struct port_f11_volume vol;struct port_f11_fid fid;
        uint8_t ini_header[BLOCK],test_header[BLOCK];
        uint32_t ini_lbn=0,ini_blocks=0,tmp_lbn=0,tmp_blocks=0;
        if(port_f11_mount(&io,1u,&vol)!=PORT_F11_HOME_OK||
           vol.home.cluster!=3u||vol.home.index_bitmap_lbn!=12u||
           vol.home.max_files<SYSLOA_FID||
           port_f11_open_named_file(&io,1u,&vol,"[SYSEXE]INILOA.EXE",
                 &fid,ini_header,&ini_lbn,&ini_blocks)!=PORT_F11_HOME_OK||
           fid.number!=12u||fid.sequence!=12u||
           port_f11_open_named_file(&io,1u,&vol,"[SYSEXE]SYSLOA386.EXE",
                 &fid,test_header,&tmp_lbn,&tmp_blocks)!=PORT_F11_FILE_NOT_FOUND)break;
        uint32_t header_lbn=0;
        if(port_f11_vbn_to_lbn(vol.index_header,
                vol.index_vbn_offset+SYSLOA_FID,&header_lbn)!=PORT_F11_HOME_OK||
           header_lbn<24u||header_lbn>=DISK_BLOCKS)break;
        uint8_t previous[BLOCK];
        if(!sector(&disk,header_lbn,previous,0))break;
        int free_header=1;
        for(unsigned i=0;i<BLOCK;i++)if(previous[i]){free_header=0;break;}
        if(!free_header)break;
        const uint32_t cf=vol.home.cluster;
        uint32_t data_lbn=((ini_lbn+ini_blocks+cf-1u)/cf)*cf;
        if(!free_clusters(&disk,data_lbn,blocks,cf)||fseek(image,0,SEEK_SET))break;
        int written=1;
        for(uint32_t j=0;j<blocks;j++){
            uint8_t b[BLOCK];uint32_t n=bytes-j*BLOCK;
            if(n>BLOCK)n=BLOCK;
            if(fread(b,1,n,image)!=n){written=0;break;}
            if(n<BLOCK)memset(b+n,0,BLOCK-n);
            if(!put(&disk,data_lbn+j,b)){written=0;break;}
        }
        if(!written||ferror(image)||
           !add_header(&disk,header_lbn,ini_header,data_lbn,blocks,bytes)||
           !add_directory(&disk)||!mark_index_file(&disk)||
           !mark_clusters(&disk,data_lbn,blocks,cf)||fflush(disk.f)||
           !verify(&disk,image,bytes,header_lbn,data_lbn))break;
        printf("Installed SYSLOA386.EXE: FID 13, header LBN %u, data LBN %u, %u blocks; readback verified\n",
             header_lbn,data_lbn,blocks);
        ok=1;
    }while(0);
    if(!ok)fputs("SYSLOA386 Files-11 installation/verification failed\n",stderr);
    fclose(image);fclose(disk.f);return ok?0:1;
}
