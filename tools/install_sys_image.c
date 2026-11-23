/* Install independently linked native SYS.EXE on the INIT-created ODS-2
 * Files-11 system volume. FID, directory, index map, storage allocation
 * and file header are all real disk structures; no raw-sector bypass. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "vms_boot_files11.h"
#include "vms_f11_home.h"
#define BLOCK 512u
#define DISK_BLOCKS 8064u
#define SYS_FID 11u
#define SYS_START_LBN 33u
struct disk { FILE *f; };
static uint16_t ld16(const uint8_t *p){return (uint16_t)(p[0]|((uint16_t)p[1]<<8));}
static void st16(uint8_t *p,uint16_t v){p[0]=(uint8_t)v;p[1]=(uint8_t)(v>>8);}
static void st32(uint8_t *p,uint32_t v){st16(p,(uint16_t)v);st16(p+2,(uint16_t)(v>>16));}
static int block_io(struct disk *d,uint32_t lbn,uint8_t b[512],int write){
    if(lbn>=DISK_BLOCKS||fseek(d->f,(long)lbn*BLOCK,SEEK_SET))return 0;
    return (write?fwrite(b,1,BLOCK,d->f):fread(b,1,BLOCK,d->f))==BLOCK;
}
static uint32_t driver_read(void *ctx,uint32_t mode,uint32_t fn,
                             uint32_t lbn,uint32_t bytes,void *dest){
    return !mode&&fn==1u&&bytes==512u&&dest&&
        block_io((struct disk *)ctx,lbn,(uint8_t *)dest,0)?
        BOO_IO_NORMAL:BOO_IO_FAILED;
}
static int put(struct disk *d,uint32_t lbn,const uint8_t b[512]){
    uint8_t tmp[512];memcpy(tmp,b,BLOCK);return block_io(d,lbn,tmp,1);
}
static int available(struct disk *d,uint32_t start,uint32_t blocks,
                     uint32_t cluster){
    uint8_t bitmap[512];
    if(!cluster||start%cluster||!block_io(d,25u,bitmap,0))return 0;
    uint32_t clusters=(blocks+cluster-1u)/cluster;
    for(uint32_t i=0;i<clusters;i++){
        uint32_t bit=start/cluster+i;
        if(bit>=4096u||!(bitmap[bit/8u]&(1u<<(bit%8u))))return 0;
    }
    return 1;
}
static int occupy(struct disk *d,uint32_t start,uint32_t blocks,
                  uint32_t cluster){
    uint8_t bitmap[512];
    if(!available(d,start,blocks,cluster)||
       !block_io(d,25u,bitmap,0))return 0;
    for(uint32_t i=0;i<(blocks+cluster-1u)/cluster;i++){
        uint32_t bit=start/cluster+i;
        bitmap[bit/8u]&=(uint8_t)~(1u<<(bit%8u));
    }
    return put(d,25u,bitmap);
}
static int add_header(struct disk *d,uint32_t blocks,uint32_t start,
                      uint32_t exact_bytes){
    uint8_t b[512];
    /* This is the original INIT_INDEX FH2 template, not a fake header. */
    if(!block_io(d,16u,b,0)||b[7]!=2u||b[1]!=65u ||
       ld16(b+8u)!=4u||ld16(b+10u)!=4u)return 0;
    st16(b+8u,SYS_FID);st16(b+10u,SYS_FID);
    b[20u]=1u;b[21u]=0u; /* sequential binary record stream */
    st32(b+24u,blocks<<16u);
    st32(b+28u,(blocks+1u)<<16u);
    st16(b+32u,(uint16_t)(exact_bytes%BLOCK));
    memset(b+76u,' ',20u);
    memcpy(b+76u,"SYS.EXE;1",9u);
    memset(b+130u,0,380u);
    if(blocks<=256u && start<(1u<<22u)){
        st16(b+130u,(uint16_t)(0x4000u|((start>>16u)&63u)<<8u|
                                (blocks-1u)));
        st16(b+132u,(uint16_t)start);b[58u]=2u;
    }else if(blocks<=16384u){
        st16(b+130u,(uint16_t)(0x8000u|(blocks-1u)));
        st32(b+132u,start);b[58u]=3u;
    }else return 0;
    port_f11_checksum2(b,510u);
    return put(d,23u,b);/* INDEXF.SYS VBN 23, FID 11 */
}
static int add_directory_entry(struct disk *d){
    uint8_t b[512];
    if(!block_io(d,30u,b,0)||ld16(b)!=0xffffu)return 0;
    memset(b,0,512u);
    st16(b,20u);st16(b+2u,1u);b[5u]=7u;
    memcpy(b+6u,"SYS.EXE",7u);
    st16(b+14u,1u);st16(b+16u,SYS_FID);
    st16(b+18u,SYS_FID);st16(b+20u,0u);
    st16(b+22u,0xffffu);
    return put(d,30u,b);
}
static int mark_index(struct disk *d){
    uint8_t b[512];
    if(!block_io(d,12u,b,0)||ld16(b)!=0x03ffu)return 0;
    st16(b,0x07ffu);
    return put(d,12u,b);
}
static int check_image(struct disk *d,FILE *source,uint32_t bytes){
    const struct boo_io_driver io={driver_read,d,0u};
    struct port_f11_volume vol;
    if(port_f11_mount(&io,1u,&vol)!=PORT_F11_HOME_OK)return 0;
    struct port_f11_fid fid;uint8_t header[512];uint32_t start=0,blocks=0;
    if(port_f11_open_named_file(&io,1u,&vol,
         "SYS$SYSDEVICE:[SYSEXE]SYS.EXE",&fid,header,&start,&blocks)!=
         PORT_F11_HOME_OK||fid.number!=SYS_FID||fid.sequence!=SYS_FID||
         start!=SYS_START_LBN||blocks!=(bytes+511u)/512u)return 0;
    if(fseek(source,0,SEEK_SET))return 0;
    for(uint32_t pos=0;pos<bytes;){
        uint8_t diskbytes[512],filebytes[512];
        uint32_t n=bytes-pos;
        if(n>512u)n=512u;
        if(port_f11_read_vbn(&io,1u,header,pos/512u+1u,diskbytes)!=
           PORT_F11_HOME_OK||fread(filebytes,1,n,source)!=n||
           memcmp(diskbytes,filebytes,n))return 0;
        pos+=n;
    }
    return 1;
}
int main(int argc,char **argv){
    if(argc!=3){fprintf(stderr,"usage: %s portdisk.img sys.exe\n",argv[0]);return 2;}
    struct disk d={fopen(argv[1],"r+b")};
    FILE *src=fopen(argv[2],"rb");
    if(!d.f||!src){perror("open");if(d.f)fclose(d.f);if(src)fclose(src);return 2;}
    int ok=0;
    do{
        if(fseek(src,0,SEEK_END))break;
        long len=ftell(src);
        /* Native SYS.EXE may cross 256-block format-1 map limit.
         * FH2 format-2 map supports 16384 blocks; physical volume and
         * allocator bounds must also be observed before copying. */
        if(len<512L||len>16384L*BLOCK||
           len>(long)(DISK_BLOCKS-SYS_START_LBN)*BLOCK)break;
        uint32_t bytes=(uint32_t)len,blocks=(bytes+511u)/512u;
        const struct boo_io_driver io={driver_read,&d,0u};
        struct port_f11_volume vol;struct port_f11_fid existing;
        uint8_t header[512];uint32_t start=0,oldblocks=0;
        if(port_f11_mount(&io,1u,&vol)!=PORT_F11_HOME_OK||
           vol.home.cluster!=3u||vol.home.index_bitmap_lbn!=12u||
           vol.home.max_files<11u||
           !available(&d,SYS_START_LBN,blocks,vol.home.cluster))break;
        if(port_f11_open_named_file(&io,1u,&vol,"[SYSEXE]SYS.EXE",
            &existing,header,&start,&oldblocks)!=PORT_F11_FILE_NOT_FOUND)break;
        if(!block_io(&d,23u,header,0))break;
        /* An unused INDEXF header slot must contain no prior allocation. */
        if(ld16(header+8u)!=0u ||ld16(header+10u)!=0u)break;
        if(fseek(src,0,SEEK_SET))break;
        uint8_t b[512];
        int data_ok=1;
        for(uint32_t i=0;i<blocks;i++){
            uint32_t want=bytes-i*BLOCK;
            if(want>BLOCK)want=BLOCK;
            size_t n=fread(b,1,want,src);
            if(n!=want){data_ok=0;break;}
            if(n<512u)memset(b+n,0,512u-n);
            if(!put(&d,SYS_START_LBN+i,b)){data_ok=0;break;}
        }
        if(!data_ok || ferror(src))break;
        if(!add_header(&d,blocks,SYS_START_LBN,bytes)||
           !mark_index(&d)||!add_directory_entry(&d)||
           !occupy(&d,SYS_START_LBN,blocks,vol.home.cluster)||
           fflush(d.f)||!check_image(&d,src,bytes))break;
        printf("Installed SYS.EXE: FID %u, LBN %u, %u blocks, %u bytes; readback verified\n",
               SYS_FID,SYS_START_LBN,blocks,bytes);
        ok=1;
    }while(0);
    if(!ok)fprintf(stderr,"SYS.EXE Files-11 installation/verification failed\n");
    fclose(src);fclose(d.f);
    return ok?0:1;
}
