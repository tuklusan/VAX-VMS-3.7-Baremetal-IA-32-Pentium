#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "vms_f11_bootdir.h"
#define CHECK(x) do {if(!(x)){fprintf(stderr,"F11 fragmented directory %d: %s\n",__LINE__,#x);return 1;}}while(0)
struct fake {uint8_t first[512],last[512];unsigned calls,fail_at;};
static uint32_t read_fake(void *p,uint32_t lbn,uint8_t out[512]){
    struct fake *f=p;f->calls++;
    if(f->calls==f->fail_at)return PORT_DQ_DIR_MAP;
    if(lbn==700u){memcpy(out,f->first,512u);return PORT_DQ_DIR_OK;}
    if(lbn==1234u){memcpy(out,f->last,512u);return PORT_DQ_DIR_OK;}
    return PORT_DQ_DIR_MAP;
}
static int diskread(FILE *disk,uint32_t lbn,uint8_t out[512]){
    return fseek(disk,(long)lbn*512L,SEEK_SET)==0&&fread(out,1,512u,disk)==512u;
}
static void put16(uint8_t *p,unsigned o,uint16_t v){p[o]=(uint8_t)v;p[o+1u]=(uint8_t)(v>>8u);}
static void checksum(uint8_t *b){uint16_t sum=0u;for(unsigned i=0;i<510u;i+=2u)sum=(uint16_t)(sum+port_dq_index_word(b,i));put16(b,510u,sum);}
struct seen {unsigned total,exe,ini,loa;};
static uint32_t record(void *p,const uint8_t *n,size_t len,uint16_t version,
                       const struct port_dq_dir_fid *id){
    struct seen *s=p;s->total++;
    if(version!=1u)return 0u;
    if(len==7u&&!memcmp(n,"SYS.EXE",7u)&&id->number==11u)s->exe++;
    if(len==10u&&!memcmp(n,"INILOA.EXE",10u)&&id->number==12u)s->ini++;
    if(len==13u&&!memcmp(n,"SYSLOA386.EXE",13u)&&id->number==13u)s->loa++;
    return 1u;
}
int main(void){
    FILE *disk=fopen("portdisk.img","rb");CHECK(disk);
    uint8_t home[512],index[512],mfd[512],fh2[512],real[512];
    uint32_t lbn=0u,vbn=0u;
    CHECK(diskread(disk,1u,home));
    CHECK(port_dq_index_location(home,&lbn)==PORT_DQ_INDEX_OK);
    CHECK(diskread(disk,lbn,index));
    CHECK(port_dq_mfd_location(home,index,&vbn,&lbn)==PORT_DQ_MFD_OK);
    CHECK(diskread(disk,lbn,mfd));
    const struct port_dq_dir_fid dir={10u,10u,0u};
    CHECK(port_dq_image_header_lbn(home,index,&dir,&lbn)==PORT_DQ_IMAGE_OK);
    CHECK(diskread(disk,lbn,fh2));
    CHECK(port_dq_dir_map_vbn(fh2,1u,&lbn)==PORT_DQ_DIR_OK);
    CHECK(diskread(disk,lbn,real));
    struct fake f={0};
    memcpy(f.last,real,512u);
    put16(f.first,0u,UINT16_MAX);
    const unsigned p=(unsigned)fh2[1u]*2u;
    CHECK(p+8u<510u);
    fh2[58u]=4u;
    put16(fh2,p,UINT16_C(0x4000));put16(fh2,p+2u,700u);
    put16(fh2,p+4u,UINT16_C(0x4000));put16(fh2,p+6u,1234u);
    checksum(fh2);
    struct port_f11_boot_volume volume={home,index,mfd,read_fake,&f,2u};
    uint32_t matched=0u,entries=0u;struct port_dq_dir_fid fid={0};
    CHECK(port_f11_boot_lookup(&volume,fh2,"SYS.EXE",7u,1u,&fid,
           &matched)==PORT_F11_PATH_OK);
    CHECK(matched==2u&&fid.number==11u&&f.calls==2u);
    struct seen observed={0};f.calls=0u;
    CHECK(port_f11_bootdir_walk(&volume,fh2,record,&observed,
           &entries)==PORT_F11_BOOTDIR_OK);
    CHECK(f.calls==2u&&entries==3u&&observed.total==3u);
    CHECK(observed.exe==1u&&observed.ini==1u&&observed.loa==1u);
    for(unsigned fail=1u;fail<=2u;fail++){
        f.fail_at=fail;f.calls=0u;entries=UINT32_MAX;
        memset(&observed,0,sizeof observed);
        CHECK(port_f11_bootdir_walk(&volume,fh2,record,&observed,
              &entries)==PORT_F11_BOOTDIR_MAP);
        CHECK(entries==(fail==1u?0u:0u));
        f.calls=0u;matched=0xcafebabeu;fid.number=77u;
        CHECK(port_f11_boot_lookup(&volume,fh2,"SYS.EXE",7u,1u,&fid,
             &matched)==PORT_F11_PATH_MAP);
        CHECK(matched==0xcafebabeu&&fid.number==77u);
    }
    CHECK(fclose(disk)==0);
    puts("FIL$FINDFILID/FIL$READVBN fragmented two-VBN physical directory and rollback PASS");
    return 0;
}
