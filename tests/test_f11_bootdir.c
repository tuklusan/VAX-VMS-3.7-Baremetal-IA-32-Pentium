#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "vms_f11_bootdir.h"
#define CHECK(x) do{if(!(x)){fprintf(stderr,"FIL$FINDFILID enumerator %d: %s\n",__LINE__,#x);return 1;}}while(0)
struct disk {FILE *f;unsigned reads,fail_at;};
static uint32_t readsec(void *o,uint32_t lbn,uint8_t out[512]){
    struct disk *d=o;
    if(d->reads++==d->fail_at||lbn>8000u||
       fseek(d->f,(long)lbn*512L,SEEK_SET)||
       fread(out,1,512u,d->f)!=512u)return PORT_DQ_DIR_MAP;
    return PORT_DQ_DIR_OK;
}
struct found {unsigned exe,ini,loa,visits;};
static uint32_t visit(void *o,const uint8_t *name,size_t n,
    uint16_t version,const struct port_dq_dir_fid *f){
    struct found *t=o;t->visits++;
    if(n==7u&&!memcmp(name,"SYS.EXE",7u)&&version==1u){
        if(f->number==11u&&f->sequence==11u)t->exe++;
    }
    if(n==10u&&!memcmp(name,"INILOA.EXE",10u)&&version==1u){
        if(f->number==12u&&f->sequence==12u)t->ini++;
    }
    if(n==13u&&!memcmp(name,"SYSLOA386.EXE",13u)&&version==1u){
        if(f->number==13u&&f->sequence==13u)t->loa++;
    }
    return 1u;
}
static uint32_t stop(void *o,const uint8_t *name,size_t n,
    uint16_t ver,const struct port_dq_dir_fid *f){
    (void)o;(void)name;(void)n;(void)ver;(void)f;return 0u;
}
static int diskblock(FILE *f,uint32_t n,uint8_t *b){
    return fseek(f,(long)n*512L,SEEK_SET)==0&&fread(b,1,512u,f)==512u;
}
int main(void){
    FILE *f=fopen("portdisk.img","rb");CHECK(f);
    uint8_t home[512],index[512],mfd[512],sysexe[512],block[512];
    CHECK(diskblock(f,1u,home));uint32_t lbn=0u;
    CHECK(port_dq_index_location(home,&lbn)==PORT_DQ_INDEX_OK);
    CHECK(diskblock(f,lbn,index));uint32_t vbn=0u;
    CHECK(port_dq_mfd_location(home,index,&vbn,&lbn)==PORT_DQ_MFD_OK);
    CHECK(diskblock(f,lbn,mfd));
    const struct port_dq_dir_fid d={10u,10u,0u};
    CHECK(port_dq_image_header_lbn(home,index,&d,&lbn)==PORT_DQ_IMAGE_OK);
    CHECK(diskblock(f,lbn,sysexe));
    CHECK(port_dq_dir_validate_header(sysexe,&d)==PORT_DQ_DIR_OK);
    CHECK(port_dq_dir_map_vbn(sysexe,1u,&lbn)==PORT_DQ_DIR_OK);
    CHECK(diskblock(f,lbn,block));
    struct found target={0};uint32_t seen=0u;
    CHECK(port_f11_bootdir_block(block,visit,&target,&seen)==PORT_F11_BOOTDIR_OK);
    CHECK(target.exe==1u&&target.ini==1u&&target.loa==1u&&seen>=3u);
    struct disk io={f,0u,UINT32_MAX};
    const struct port_f11_boot_volume volume={home,index,mfd,readsec,&io,1u};
    memset(&target,0,sizeof target);seen=UINT32_MAX;
    CHECK(port_f11_bootdir_walk(&volume,sysexe,visit,&target,&seen)==PORT_F11_BOOTDIR_OK);
    CHECK(seen>=3u&&target.visits==seen&&target.exe==1u&&target.ini==1u&&target.loa==1u);
    CHECK(io.reads==1u);
    CHECK(port_f11_bootdir_walk(&volume,sysexe,stop,NULL,&seen)==PORT_F11_BOOTDIR_STOP);
    CHECK(seen==1u);
    io.fail_at=io.reads;
    CHECK(port_f11_bootdir_walk(&volume,sysexe,visit,&target,&seen)==PORT_F11_BOOTDIR_MAP);
    CHECK(seen==0u);
    CHECK(port_f11_bootdir_block(NULL,visit,&target,&seen)==PORT_F11_BOOTDIR_BAD);
    CHECK(port_f11_bootdir_block(block,NULL,&target,&seen)==PORT_F11_BOOTDIR_BAD);
    for(unsigned i=0;i<7u;i++){
        uint8_t damaged[512];memcpy(damaged,block,512u);
        switch(i){
        case 0:damaged[0]=0;damaged[1]=0;break;
        case 1:damaged[0]=1;damaged[1]=0;break;
        case 2:damaged[0]=0xfe;damaged[1]=0xff;break;
        case 3:damaged[5]=0;break;
        case 4:damaged[5]=255;break;
        case 5:damaged[0]=0xff;damaged[1]=1;break;
        default:damaged[0]=0xfd;damaged[1]=0xff;break;
        }
        uint32_t inspected=0u;
        CHECK(port_f11_bootdir_block(damaged,visit,&target,&inspected)==PORT_F11_BOOTDIR_CORRUPT);
    }
    CHECK(fclose(f)==0);
    puts("FIL$FINDFILID physical Files-11 directory enumeration, 3 real image FIDs PASS");
    return 0;
}
