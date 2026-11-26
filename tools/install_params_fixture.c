/* Add one synthetic, structurally valid ODS-2 SYSBOOT PARAMS.DAT record to
 * a scratch test disk. This fixture is never part of the normal portdisk. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "vms_boot_files11.h"
#include "vms_sysboot_paramfile.h"
#include "vms_f11_home.h"

#define BLOCK 512u
#define DISK_BLOCKS 8064u
#define PARAM_FID 14u
struct disk { FILE *file; };
static uint16_t get16(const uint8_t *p){return (uint16_t)(p[0]|((uint16_t)p[1]<<8));}
static void put16(uint8_t *p,uint16_t v){p[0]=(uint8_t)v;p[1]=(uint8_t)(v>>8);}
static void put32(uint8_t *p,uint32_t v){put16(p,(uint16_t)v);put16(p+2,(uint16_t)(v>>16));}
static int block(struct disk *d,uint32_t lbn,uint8_t b[BLOCK],int write){
    return d&&d->file&&lbn<DISK_BLOCKS&&
        !fseek(d->file,(long)lbn*BLOCK,SEEK_SET)&&
        (write?fwrite(b,1,BLOCK,d->file):fread(b,1,BLOCK,d->file))==BLOCK;
}
static int put_block(struct disk *d,uint32_t lbn,const uint8_t b[BLOCK]){
    uint8_t copy[BLOCK];memcpy(copy,b,BLOCK);return block(d,lbn,copy,1);
}
static uint32_t read_disk(void *ctx,uint32_t mode,uint32_t fn,uint32_t lbn,
                          uint32_t bytes,void *dst){
    return !mode&&fn==1u&&bytes==BLOCK&&dst&&block(ctx,lbn,dst,0)?
        BOO_IO_NORMAL:BOO_IO_FAILED;
}
static int insert_entry(struct disk *d,const struct port_f11_volume *v,
                        const struct port_f11_fid directory){
    uint8_t header[BLOCK],data[BLOCK];
    if(port_f11_read_file_header(&(struct boo_io_driver){read_disk,d,0u},1u,
          v,directory,header)!=PORT_F11_HOME_OK||
       port_f11_read_vbn(&(struct boo_io_driver){read_disk,d,0u},1u,
          header,1u,data)!=PORT_F11_HOME_OK)return 0;
    unsigned at=0u,insert=UINT32_MAX,terminator=0u;
    while(at+2u<=BLOCK){
        const uint16_t size=get16(data+at);
        if(size==0xffffu){terminator=at;break;}
        if(size<12u||at+2u+size>BLOCK)return 0;
        const uint8_t n=data[at+5u];
        if(!n||at+6u+n>at+2u+size)return 0;
        static const char wanted[]="PARAMS.DAT";
        int greater=0;
        for(unsigned i=0;i<n&&i<sizeof wanted-1u;i++){
            uint8_t a=data[at+6u+i],b=(uint8_t)wanted[i];
            if(a>='a'&&a<='z')a=(uint8_t)(a-'a'+'A');
            if(b>='a'&&b<='z')b=(uint8_t)(b-'a'+'A');
            if(a!=b){greater=a>b;break;}
        }
        if(greater&&insert==UINT32_MAX)insert=at;
        at+=2u+size;
    }
    if(at+2u>BLOCK||get16(data+at)!=0xffffu)return 0;
    if(insert==UINT32_MAX)insert=terminator;
    const unsigned tail=terminator-insert+2u;
    if(insert+24u+tail>BLOCK)return 0;
    memmove(data+insert+24u,data+insert,tail);
    memset(data+insert,0,24u);
    put16(data+insert,22u);put16(data+insert+2u,1u);
    data[insert+5u]=10u;memcpy(data+insert+6u,"PARAMS.DAT",10u);
    put16(data+insert+16u,1u);put16(data+insert+18u,PARAM_FID);
    put16(data+insert+20u,PARAM_FID);put16(data+insert+22u,0u);
    return put_block(d,30u,data);
}
static int build_parameter_block(uint8_t data[BLOCK]){
    memset(data,0,BLOCK);
    const unsigned at=32u;
    data[at]=8u;memcpy(data+at+1u,"IRPCOUNT",8u);
    put32(data+at+16u,61u);
    data[at+20u]=14u;memcpy(data+at+21u,"TIMEPROMPTWAIT",14u);
    put32(data+at+36u,65535u);
    /* Zero longword at offset 72 terminates BOO$USEFILE's record scan. */
    return 1;
}
static int add_file(struct disk *d,const struct port_f11_volume *v){
    const struct boo_io_driver io={read_disk,d,0u};
    if(v->home.max_files<=PARAM_FID)return 0;
    uint32_t header_lbn=0u;
    if(port_f11_vbn_to_lbn(v->index_header,
          v->index_vbn_offset+PARAM_FID,&header_lbn)!=PORT_F11_HOME_OK)
        {fputs("fixture: FID header map failed\n",stderr);return 0;}
    uint8_t header[BLOCK];
    if(!block(d,header_lbn,header,0))
        {fputs("fixture: cannot read free FID header\n",stderr);return 0;}
    for(unsigned i=0;i<BLOCK;i++)if(header[i])
        {fprintf(stderr,"fixture: FID header %u already allocated\n",header_lbn);return 0;}
    const uint32_t cluster=v->home.cluster;
    uint8_t bitmap[BLOCK];
    if(!cluster||!block(d,25u,bitmap,0))
        {fputs("fixture: cannot read free-space bitmap\n",stderr);return 0;}
    uint32_t data_lbn=UINT32_MAX;
    for(uint32_t bit=0;bit<4096u;bit++){
        const uint32_t candidate=bit*cluster;
        if(candidate+cluster>DISK_BLOCKS)break;
        if(bitmap[bit/8u]&(1u<<(bit%8u))){data_lbn=candidate;break;}
    }
    if(data_lbn==UINT32_MAX){fputs("fixture: no free data cluster\n",stderr);return 0;}
    uint8_t payload[BLOCK];build_parameter_block(payload);
    if(!put_block(d,data_lbn,payload))return 0;
    memset(header,0,BLOCK);
    if(!block(d,16u,header,0)||header[7u]!=2u||header[1u]!=65u||
       get16(header+8u)!=4u||get16(header+10u)!=4u)
        {fputs("fixture: file-header template invalid\n",stderr);return 0;}
    put16(header+8u,PARAM_FID);put16(header+10u,PARAM_FID);
    header[20u]=1u;header[21u]=0u;
    put32(header+24u,1u<<16u);put32(header+28u,2u<<16u);
    put16(header+32u,76u);
    memset(header+76u,' ',20u);memcpy(header+76u,"PARAMS.DAT;1",12u);
    memset(header+130u,0,380u);
    put16(header+130u,(uint16_t)(0x4000u|((data_lbn>>16u)&0x3fu)<<8u));
    put16(header+132u,(uint16_t)data_lbn);header[58u]=2u;
    port_f11_checksum2(header,510u);
    uint8_t index_bitmap[BLOCK];
    const unsigned index_bit=PARAM_FID-1u;
    if(!block(d,v->home.index_bitmap_lbn,index_bitmap,0)||
       (index_bitmap[index_bit/8u]&(1u<<(index_bit%8u)))||
       !put_block(d,header_lbn,header))
        {fputs("fixture: index bitmap or FID-header write failed\n",stderr);return 0;}
    index_bitmap[index_bit/8u]|=(uint8_t)(1u<<(index_bit%8u));
    if(!put_block(d,v->home.index_bitmap_lbn,index_bitmap))return 0;
    bitmap[data_lbn/cluster/8u]&=(uint8_t)~(1u<<((data_lbn/cluster)%8u));
    if(!put_block(d,25u,bitmap))
        {fputs("fixture: data allocation bitmap update failed\n",stderr);return 0;}
    if(!insert_entry(d,v,(struct port_f11_fid){10u,10u,0u}))
        {fputs("fixture: SYSEXE directory update failed\n",stderr);return 0;}
    if(fflush(d->file))
        {fputs("fixture: disk flush failed\n",stderr);return 0;}
    struct port_f11_volume check_volume;
    if(port_f11_mount(&io,1u,&check_volume)!=PORT_F11_HOME_OK)
        {fputs("fixture: updated volume failed mount\n",stderr);return 0;}
    struct port_f11_fid opened={0};uint8_t opened_header[BLOCK];
    uint32_t opened_first=0u,opened_blocks=0u;
    uint32_t open_status=port_f11_open_named_file(&io,1u,&check_volume,
        "SYS$SYSDEVICE:[SYSEXE]PARAMS.DAT",&opened,opened_header,
        &opened_first,&opened_blocks);
    if(open_status!=PORT_F11_HOME_OK)
        {fprintf(stderr,"fixture: named PARAMS.DAT open status=%u\n",open_status);return 0;}
    struct port_vms37_init_params params;
    uint8_t startup[32],readback[BLOCK];uint32_t applied=0u;size_t bytes=0u;
    uint32_t use_status=port_vms37_init_param_defaults(&params);
    if(use_status==PORT_VMS37_SYSPARAM_OK)
        use_status=port_sysboot_usefile(&io,1u,&check_volume,
          "SYS$SYSDEVICE:[SYSEXE]PARAMS.DAT",&params,startup,&applied,
          readback,sizeof readback,&bytes);
    if(use_status!=PORT_SYSBOOT_USEFILE_OK||params.irp_count!=61u||
       params.terminal_prompt_wait!=-1||applied!=2u||bytes!=BLOCK)
        {fprintf(stderr,"fixture: BOO$USEFILE readback failed status=%u IRP=%u count=%u bytes=%zu\n",use_status,params.irp_count,applied,bytes);return 0;}
    printf("Installed scratch Files-11 PARAMS.DAT: FID %u IRPCOUNT=61 TIMEPROMPTWAIT=65535; BOO$USEFILE readback verified\n",PARAM_FID);
    return 1;
}
int main(int argc,char **argv){
    if(argc!=2){fprintf(stderr,"usage: %s scratch-portdisk.img\n",argv[0]);return 2;}
    struct disk d={fopen(argv[1],"r+b")};
    if(!d.file){perror("open");return 2;}
    const struct boo_io_driver io={read_disk,&d,0u};
    struct port_f11_volume volume;
    uint32_t mount=port_f11_mount(&io,1u,&volume);
    uint32_t exists=PORT_F11_HOME_BADARG;
    if(mount==PORT_F11_HOME_OK)
        exists=port_f11_open_named_file(&io,1u,&volume,
          "SYS$SYSDEVICE:[SYSEXE]PARAMS.DAT",&(struct port_f11_fid){0},
          (uint8_t[BLOCK]){0},&(uint32_t){0},&(uint32_t){0});
    if(mount!=PORT_F11_HOME_OK)
        fprintf(stderr,"fixture: mount status %u\n",mount);
    else if(exists!=PORT_F11_FILE_NOT_FOUND)
        fprintf(stderr,"fixture: expected absent PARAMS.DAT, got status %u\n",exists);
    int ok=mount==PORT_F11_HOME_OK&&exists==PORT_F11_FILE_NOT_FOUND&&
        add_file(&d,&volume);
    if(!ok)fputs("scratch PARAMS.DAT Files-11 fixture installation failed\n",stderr);
    fclose(d.file);return ok?0:1;
}
