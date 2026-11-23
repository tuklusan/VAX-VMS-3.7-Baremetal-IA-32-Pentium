#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "vms_f11_bootpath.h"
#define CHECK(x) do{if(!(x)){fprintf(stderr,"FIL$FINDFILID/FIL$READVBN line %d: %s\n",__LINE__,#x);return 1;}}while(0)
struct disk {FILE *f;uint32_t reads,fail_at, fail_lbn;};
static uint32_t disk_read(void *p,uint32_t lbn,uint8_t sector[512]){
    struct disk *d=p;
    if(!d||!sector||!lbn||d->reads++==d->fail_at||lbn==d->fail_lbn||
       lbn>100000u||fseek(d->f,(long)lbn*512L,SEEK_SET)||
       fread(sector,1,512u,d->f)!=512u)return PORT_DQ_DIR_MAP;
    return PORT_DQ_DIR_OK;
}
static int read_lbn(FILE *f,uint32_t lbn,uint8_t bytes[512]){
    return fseek(f,(long)lbn*512L,SEEK_SET)==0&&fread(bytes,1,512u,f)==512u;
}
static void poison(struct port_f11_boot_file *f){memset(f,0xa5,sizeof *f);}
static int poisoned(const struct port_f11_boot_file *f){
    const uint8_t *p=(const uint8_t *)f;
    for(size_t i=0;i<sizeof *f;i++)if(p[i]!=0xa5)return 0;
    return 1;
}
static int test_disk(FILE *f){
    uint8_t home[512],index[512],mfd[512];
    CHECK(read_lbn(f,1u,home));
    uint32_t index_lbn=0u,mfd_lbn=0u,mfd_vbn=0u;
    CHECK(port_dq_index_location(home,&index_lbn)==PORT_DQ_INDEX_OK);
    CHECK(read_lbn(f,index_lbn,index));
    CHECK(port_dq_mfd_location(home,index,&mfd_vbn,&mfd_lbn)==PORT_DQ_MFD_OK);
    CHECK(read_lbn(f,mfd_lbn,mfd));
    struct disk d={f,0u,UINT32_MAX,UINT32_MAX};
    struct port_f11_boot_volume v={home,index,mfd,disk_read,&d,1u};
    const struct {const char *name;uint16_t file_number;} names[]={
       {"SYS.EXE",11u},{"INILOA.EXE",12u},{"SYSLOA386.EXE",13u}};
    for(size_t j=0;j<sizeof names/sizeof names[0];j++){
        struct port_f11_boot_file out;poison(&out);
        const uint32_t st=port_f11_boot_open(&v,"SYSEXE.DIR",10u,
           names[j].name,strlen(names[j].name),1u,&out);
        CHECK(st==PORT_F11_PATH_OK);
        CHECK(out.directory.number==10u&&out.directory.sequence==10u);
        CHECK(out.file.number==names[j].file_number);
        CHECK(out.file.sequence==names[j].file_number&&out.file.volume==0u);
        CHECK(out.directory_vbn==1u&&out.file_vbn==1u);
        CHECK(port_dq_image_verify_fh2(out.header,&out.file)==PORT_DQ_IMAGE_OK);
        uint8_t content[1792];memset(content,0xcd,sizeof content);
        uint32_t done=UINT32_MAX;
        CHECK(port_f11_boot_read(&v,&out,399u,1792u,content,
                sizeof content,&done)==PORT_F11_PATH_OK);
        CHECK(done==1792u);
        for(unsigned i=0;i<1792u;i++){
            uint32_t lbn=0u;
            CHECK(port_dq_dir_map_vbn(out.header,(399u+i)/512u+1u,&lbn)==PORT_DQ_DIR_OK);
            uint8_t sec[512];CHECK(read_lbn(f,lbn,sec));
            CHECK(content[i]==sec[(399u+i)%512u]);
        }
        CHECK(d.reads>=8u);
        const uint32_t old=d.reads;
        CHECK(port_f11_boot_read(&v,&out,512u,0u,content,
                sizeof content,&done)==PORT_F11_PATH_OK);
        CHECK(done==0u&&d.reads==old);
    }
    struct port_f11_boot_file out;poison(&out);
    CHECK(port_f11_boot_open(&v,"SYSEXE.DIR",10u,"SYS.EXE",7u,
           2u,&out)==PORT_F11_PATH_MISSING);
    CHECK(poisoned(&out));
    CHECK(port_f11_boot_open(&v,"DOESNOT.DIR",11u,"SYS.EXE",7u,
           1u,&out)==PORT_F11_PATH_MISSING);
    CHECK(poisoned(&out));
    CHECK(port_f11_boot_open(&v,"SYSEXE.DIR",10u,"NONEXIST.EXE",12u,
           1u,&out)==PORT_F11_PATH_MISSING);
    CHECK(poisoned(&out));
    const char *invalid[]={"","../SYS.EXE","SYS.EXE;1","SYS/EXE",
                           "SYS\\EXE","SYS EXE","SYS\nEXE"};
    for(unsigned i=0;i<sizeof invalid/sizeof invalid[0];i++){
        const char *s=invalid[i];
        CHECK(port_f11_boot_validate_name(s,strlen(s))==PORT_F11_PATH_BAD);
        CHECK(port_f11_boot_open(&v,"SYSEXE.DIR",10u,s,strlen(s),
                 1u,&out)==PORT_F11_PATH_BAD);
        CHECK(poisoned(&out));
    }
    CHECK(port_f11_boot_validate_name("SYS.EXE",7u)==PORT_F11_PATH_OK);
    CHECK(port_f11_boot_validate_name(NULL,7u)==PORT_F11_PATH_BAD);
    CHECK(port_f11_boot_validate_name("SYS.EXE",80u)==PORT_F11_PATH_BAD);
    CHECK(port_f11_boot_open(&v,"SYSEXE.DIR",10u,"SYS.EXE",7u,
           0u,&out)==PORT_F11_PATH_BAD);
    CHECK(poisoned(&out));
    CHECK(port_f11_boot_open(NULL,"SYSEXE.DIR",10u,"SYS.EXE",7u,
           1u,&out)==PORT_F11_PATH_BAD);
    CHECK(poisoned(&out));
    for(uint32_t fail=0u;fail<4u;fail++){
        d.reads=0u;d.fail_at=fail;
        CHECK(port_f11_boot_open(&v,"SYSEXE.DIR",10u,"SYS.EXE",7u,
               1u,&out)!=PORT_F11_PATH_OK);
        CHECK(poisoned(&out));
    }
    d.fail_at=UINT32_MAX;d.reads=0u;
    CHECK(port_f11_boot_open(&v,"sysexe.dir",10u,"sys.exe",7u,
           1u,&out)==PORT_F11_PATH_OK);
    CHECK(out.file.number==11u);
    uint8_t dest[1200];memset(dest,0xa5,sizeof dest);uint32_t done=0xcafebabeu;
    CHECK(port_f11_boot_read(&v,&out,10u,1201u,dest,
                sizeof dest,&done)==PORT_F11_PATH_BAD);
    CHECK(done==0xcafebabeu&&dest[0]==0xa5);
    d.fail_at=d.reads+1u;
    CHECK(port_f11_boot_read(&v,&out,400u,900u,dest,
                sizeof dest,&done)==PORT_F11_PATH_IO);
    CHECK(done==112u);
    d.fail_at=UINT32_MAX;
    CHECK(port_f11_boot_read(&v,&out,UINT32_MAX-100u,512u,dest,
                sizeof dest,&done)==PORT_F11_PATH_BAD);
    uint8_t bad[512];memcpy(bad,mfd,512u);bad[510u]^=1u;
    v.mfd=bad;poison(&out);
    CHECK(port_f11_boot_open(&v,"SYSEXE.DIR",10u,"SYS.EXE",7u,
            1u,&out)==PORT_F11_PATH_HEADER);
    CHECK(poisoned(&out));
    CHECK(d.reads>0u);
    return 0;
}
int main(void){
    FILE *f=fopen("portdisk.img","rb");CHECK(f);
    CHECK(test_disk(f)==0);
    CHECK(!fclose(f));
    puts("FIL$FINDFILID/FIL$RDCHKFILHDR/FIL$READVBN native open + real 3-file access PASS");
    return 0;
}
