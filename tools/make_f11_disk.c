/* Build a Files-11 ODS-2 blank volume with the original INIT_INDEX,
 * INIT_MFD and INIT_BITMAP paths. The volume contains no SYS.EXE until
 * the SYSBOOT image and file installation code is ported. */
#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include "vms_init_index.h"
#include "vms_init_mfd.h"
#include "vms_init_bitmap.h"
#include "vms_boot_files11.h"
#include "vms_f11_home.h"

#define VOLUME_SECTORS (8u*16u*63u)
struct file_disk { FILE *f; };
static uint32_t disk_write(void *ctx,uint32_t lbn,const uint8_t block[512]) {
    struct file_disk *disk=(struct file_disk *)ctx;
    if(lbn>=VOLUME_SECTORS || fseek(disk->f,(long)lbn*512L,SEEK_SET)!=0 ||
       fwrite(block,1,512,disk->f)!=512u)return PORT_INIT_MFD_WRITE_FAILED;
    return PORT_INIT_MFD_NORMAL;
}
static uint32_t disk_read(void *ctx,uint32_t mode,uint32_t fn,
                          uint32_t lbn,uint32_t bytes,void *block) {
    struct file_disk *disk=(struct file_disk *)ctx;
    if(mode!=0u || fn!=1u || bytes!=512u || lbn>=VOLUME_SECTORS ||
       fseek(disk->f,(long)lbn*512L,SEEK_SET)!=0 ||
       fread(block,1,512,disk->f)!=512u)return BOO_IO_FAILED;
    return BOO_IO_NORMAL;
}

static uint16_t load16(const uint8_t *p){return (uint16_t)(p[0]|((uint16_t)p[1]<<8u));}
static void store16(uint8_t *p,uint16_t v){p[0]=(uint8_t)v;p[1]=(uint8_t)(v>>8u);}
static void store32(uint8_t *p,uint32_t v){store16(p,(uint16_t)v);store16(p+2u,(uint16_t)(v>>16u));}
static int read_block(struct file_disk *disk,uint32_t lbn,uint8_t b[512]){
    return disk_read(disk,0u,1u,lbn,512u,b)==BOO_IO_NORMAL;
}
static int write_block(struct file_disk *disk,uint32_t lbn,const uint8_t b[512]){
    return disk_write(disk,lbn,b)==PORT_INIT_MFD_NORMAL;
}
/* A real system volume has a SYSEXE.DIR. INIT_MFD writes only the nine
 * reserved files; subsequent directory creation assigns a new FID, writes
 * its file header and updates INDEXF.SYS and BITMAP.SYS. No SYS.EXE is
 * invented here. */
static int make_system_directory(struct file_disk *disk){
    uint8_t b[512];
    /* File ID 10, sequence 10. Index bitmap bit 9 means in use. */
    if(!read_block(disk,12u,b) || load16(b)!=0x1ffu)return 0;
    store16(b,0x3ffu);
    if(!write_block(disk,12u,b))return 0;

    /* The MFD keeps entries in ascending filename order. Insert the new
     * entry immediately before the existing VOLSET.SYS;1 record. */
    if(!read_block(disk,27u,b))return 0;
    unsigned pos=0,insert=0;
    while(pos+24u<512u && load16(b+pos)!=0xffffu){
        if(b[pos+5u]==10u && memcmp(b+pos+6u,"VOLSET.SYS",10u)==0){
            insert=pos;break;
        }
        pos+=(unsigned)load16(b+pos)+2u;
    }
    if(!insert || pos+24u>490u)return 0;
    /* Move VOLSET and the directory record terminator. */
    memmove(b+insert+24u,b+insert,(size_t)(pos-insert)+26u);
    store16(b+insert,22u);store16(b+insert+2u,1u);
    b[insert+4u]=0u;b[insert+5u]=10u;
    memcpy(b+insert+6u,"SYSEXE.DIR",10u);
    store16(b+insert+16u,1u);
    store16(b+insert+18u,10u);store16(b+insert+20u,10u);
    store16(b+insert+22u,0u);
    if(!write_block(disk,27u,b))return 0;

    /* Duplicate the well-formed 000000.DIR header, then replace only
     * its identity and mapped file extent. The index header is at LBN 13,
     * so FID 10 occupies index-file VBN 22 (LBN 22). */
    if(!read_block(disk,16u,b))return 0;
    if(load16(b+8u)!=4u || load16(b+10u)!=4u || b[58u]!=2u)return 0;
    store16(b+8u,10u);store16(b+10u,10u);
    memset(b+76u,' ',20u);
    memcpy(b+76u,"SYSEXE.DIR;1",12u);
    store32(b+24u,3u<<16u);store32(b+28u,2u<<16u);
    store16(b+130u,0x4002u);store16(b+132u,30u);
    port_f11_checksum2(b,510u);
    if(!write_block(disk,22u,b))return 0;

    /* A valid empty Files-11 directory has the 0xffff end marker. */
    memset(b,0,512u);store16(b,0xffffu);
    if(!write_block(disk,30u,b))return 0;
    memset(b,0,512u);
    if(!write_block(disk,31u,b)||!write_block(disk,32u,b))return 0;
    return 1;
}

static int disk_ok(struct file_disk *disk){
    const struct boo_io_driver driver={disk_read,disk,0u};
    struct port_f11_volume volume;
    if(port_f11_mount(&driver,1u,&volume)!=PORT_F11_HOME_OK)
        return 0;
    uint8_t header[512];uint32_t start=0,count=0;
    const struct port_f11_fid mfd={4u,4u,0u};
    if(port_f11_read_file_header(&driver,1u,&volume,mfd,header)!=PORT_F11_HOME_OK)
        return 0;
    if(port_f11_file_extent_stats(header,&start,&count)!=PORT_F11_HOME_OK ||
       start!=27u || count!=3u)return 0;
    struct port_f11_fid found;
    if(port_f11_find_file(&driver,1u,&volume,mfd,"SYSEXE.DIR",1u,&found)!=PORT_F11_HOME_OK ||
       found.number!=10u || found.sequence!=10u)return 0;
    if(port_f11_open_named_file(&driver,1u,&volume,"[SYSEXE]SYS.EXE",&found,
                                 header,&start,&count)!=PORT_F11_FILE_NOT_FOUND)return 0;
    if(port_f11_find_file(&driver,1u,&volume,mfd,"INDEXF.SYS",1u,&found)!=PORT_F11_HOME_OK ||
       found.number!=1u || found.sequence!=1u)return 0;
    return port_f11_open_named_file(&driver,1u,&volume,"SYS.EXE",&found,
              header,&start,&count)==PORT_F11_FILE_NOT_FOUND;
}
int main(int argc,char **argv) {
    if(argc!=2){fputs("usage: make_f11_disk output-image\n",stderr);return 2;}
    struct file_disk disk={fopen(argv[1],"w+b")};
    if(!disk.f){perror(argv[1]);return 2;}
    if(fseek(disk.f,(long)VOLUME_SECTORS*512L-1L,SEEK_SET)!=0 ||
       fputc(0,disk.f)==EOF){perror("disk size");fclose(disk.f);return 2;}
    static const struct port_init_allocation index_areas[]={
        {0u,3u},{3u,3u},{6u,3u},{9u,3u},{12u,12u}
    };
    static const struct port_init_allocation all_occupied[]={
        {0u,3u},{3u,3u},{6u,3u},{9u,3u},{12u,12u},
        {24u,3u},{27u,3u},{30u,3u}
    };
    struct port_init_index_layout layout={0};
    layout.cluster=3u;
    layout.volume_blocks=VOLUME_SECTORS;
    layout.max_files=128u;
    layout.boot_lbn=0u;layout.boot_blocks=3u;
    layout.home1_lbn=3u;layout.home2_lbn=6u;
    layout.real_home_lbn=6u;layout.alternate_index_lbn=9u;
    layout.index_lbn=12u;layout.index_blocks=12u;
    layout.bitmap_lbn=24u;layout.bitmap_blocks=3u;
    layout.mfd_lbn=27u;layout.mfd_blocks=3u;
    layout.owner_uic=0x00010004u;
    layout.file_protection=0xff00u;
    layout.window=7u;layout.lru_limit=3u;
    layout.creation_time=((uint64_t)time(0)+3506716800ull)*10000000ull;
    layout.serial=1u;
    memcpy(layout.label,"SYSVOL",6u);
    memcpy(layout.owner_name,"SYSTEM",6u);
    layout.index_areas=index_areas;
    layout.index_area_count=sizeof(index_areas)/sizeof(index_areas[0]);
    uint8_t buffer[512];
    const struct port_init_bitmap_options bitmap={0u,24u,3u,
        VOLUME_SECTORS,3u,63u,16u,8u};
    uint32_t status=port_init_index(&layout,buffer,disk_write,&disk);
    if(status&1u)status=port_init_mfd(layout.mfd_lbn,0u,buffer,disk_write,&disk);
    if(status&1u && !make_system_directory(&disk))status=PORT_INIT_MFD_WRITE_FAILED;
    if(status&1u)status=port_init_bitmap(&bitmap,all_occupied,
                    sizeof(all_occupied)/sizeof(all_occupied[0]),
                    buffer,disk_write,&disk);
    if(fflush(disk.f)!=0 || fseek(disk.f,0L,SEEK_END)!=0 ||
       ftell(disk.f)!=(long)VOLUME_SECTORS*512L ||
       !(status&1u) || !disk_ok(&disk)){
        fprintf(stderr,"Files-11 initialization/verification failed, status %u\n",status);
        fclose(disk.f);remove(argv[1]);return 1;
    }
    printf("Files-11 ODS-2 volume %u blocks, system directory exists; SYS.EXE absent\n",
        VOLUME_SECTORS);
    if(fclose(disk.f)!=0)return 1;
    return 0;
}
