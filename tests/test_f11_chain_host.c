#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "vms_f11_chain.h"
#define CHECK(x) do{if(!(x)){fprintf(stderr,"FIL$RDCHKFILHDR source %d: %s\n",__LINE__,#x);return 1;}}while(0)
struct disk {FILE *f;uint32_t reads,fail_at;};
static uint32_t fetch(void *o,uint32_t lbn,uint8_t out[512]){
    struct disk *d=o;
    if(!d||d->reads++==d->fail_at||lbn>8064u||
       fseek(d->f,(long)lbn*512L,SEEK_SET)||
       fread(out,1,512u,d->f)!=512u)return PORT_DQ_DIR_MAP;
    return PORT_DQ_DIR_OK;
}
static int at(FILE *f,uint32_t n,uint8_t b[512]){
    return !fseek(f,(long)n*512L,SEEK_SET)&&fread(b,1,512u,f)==512u;
}
int main(void){
    FILE *disk=fopen("portdisk.img","rb");CHECK(disk);
    uint8_t home[512],index[512],mfd[512];uint32_t lbn=0u,vbn=0u;
    CHECK(at(disk,1u,home));
    CHECK(port_dq_index_location(home,&lbn)==PORT_DQ_INDEX_OK);
    CHECK(at(disk,lbn,index));
    CHECK(port_dq_mfd_location(home,index,&vbn,&lbn)==PORT_DQ_MFD_OK);
    CHECK(at(disk,lbn,mfd));
    struct disk io={disk,0u,UINT32_MAX};
    const struct port_f11_boot_volume v={home,index,mfd,fetch,&io,1u};
    static const char *const names[]={"SYS.EXE","SYSLOA386.EXE","INILOA.EXE"};
    for(unsigned file=0;file<3u;file++){
        const char *name=names[file];
        struct port_f11_boot_file info;
        CHECK(port_f11_boot_open(&v,"SYSEXE.DIR",10u,name,strlen(name),
             1u,&info)==PORT_F11_PATH_OK);
        struct port_f11_chain chain={0};
        CHECK(port_f11_chain_collect(&v,&info.file,&chain)==PORT_F11_CHAIN_OK);
        CHECK(chain.first.number==info.file.number&&chain.headers==1u);
        CHECK(chain.count>=1u&&chain.blocks>=3u&&chain.first_lbn);
        for(unsigned sector=1u;sector<=3u;sector++){
            uint32_t derived=0u,direct=0u;
            CHECK(port_f11_chain_lbn(&chain,sector,&derived)==PORT_F11_CHAIN_OK);
            CHECK(port_dq_dir_map_vbn(info.header,sector,&direct)==PORT_DQ_DIR_OK);
            CHECK(derived==direct);
        }
        uint8_t received[1536];uint32_t done=0xdeadbeefu;
        CHECK(port_f11_chain_read(&v,&chain,399u,1536u,received,
             sizeof received,&done)==PORT_F11_CHAIN_OK);
        CHECK(done==1536u);
        for(unsigned k=0;k<1536u;k++){
            uint32_t actual=0u;
            CHECK(port_f11_chain_lbn(&chain,(399u+k)/512u+1u,
                  &actual)==PORT_F11_CHAIN_OK);
            uint8_t bytes[512];CHECK(at(disk,actual,bytes));
            CHECK(received[k]==bytes[(399u+k)%512u]);
        }
        CHECK(port_f11_chain_read(&v,&chain,0u,0u,received,
             sizeof received,&done)==PORT_F11_CHAIN_OK&&done==0u);
        CHECK(port_f11_chain_read(&v,&chain,0u,2000u,received,
             sizeof received,&done)==PORT_F11_CHAIN_BAD);
        CHECK(port_f11_chain_lbn(&chain,UINT32_MAX,&lbn)!=PORT_F11_CHAIN_OK);
        io.fail_at=io.reads;
        CHECK(port_f11_chain_read(&v,&chain,0u,512u,received,
             sizeof received,&done)==PORT_F11_CHAIN_IO);
        CHECK(done==0u);
        io.fail_at=UINT32_MAX;
    }
    struct port_f11_chain sentinel;
    memset(&sentinel,0xa5,sizeof sentinel);
    io.fail_at=io.reads;
    const struct port_dq_dir_fid sys={11u,11u,0u};
    CHECK(port_f11_chain_collect(&v,&sys,&sentinel)==PORT_F11_CHAIN_IO);
    CHECK(sentinel.first.number==0xa5a5u);
    io.fail_at=UINT32_MAX;
    CHECK(port_f11_chain_collect(&v,&sys,&sentinel)==PORT_F11_CHAIN_OK);
    CHECK(port_f11_chain_lbn(NULL,1u,&lbn)==PORT_F11_CHAIN_BAD);
    CHECK(port_f11_chain_lbn(&sentinel,0u,&lbn)==PORT_F11_CHAIN_BAD);
    CHECK(port_f11_chain_read(NULL,&sentinel,0u,512u,NULL,0u,&lbn)==PORT_F11_CHAIN_BAD);
    CHECK(!fclose(disk));
    puts("FIL$RDCHKFILHDR FH2 extension-chain collector and real native disk byte reads PASS");
    return 0;
}
