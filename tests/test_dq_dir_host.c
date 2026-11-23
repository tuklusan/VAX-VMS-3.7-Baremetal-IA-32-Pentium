#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "vms_dq_dir.h"
#define CHECK(x) do{if(!(x)){fprintf(stderr,"FIL$FINDFILID line %d: %s\n",__LINE__,#x);return 1;}}while(0)
static int at(FILE *f,uint32_t lbn,uint8_t b[512]){
    return fseek(f,512L*(long)lbn,SEEK_SET)==0&&fread(b,1,512u,f)==512u;
}
static void put16(uint8_t *b,unsigned i,uint16_t v){b[i]=(uint8_t)v;b[i+1u]=(uint8_t)(v>>8u);}
static void sum(uint8_t *b){uint16_t s=0u;for(unsigned i=0;i<510u;i+=2u)s=(uint16_t)(s+port_dq_index_word(b,i));put16(b,510u,s);}
int main(void){
    FILE *f=fopen("portdisk.img","rb");CHECK(f!=NULL);
    uint8_t home[512],index[512],mfd[512],mfddir[512],sysexe[512],sysexedir[512];
    CHECK(at(f,1u,home));
    uint32_t index_lbn=0u;
    CHECK(port_dq_index_location(home,&index_lbn)==PORT_DQ_INDEX_OK);
    CHECK(at(f,index_lbn,index));
    uint32_t mfd_vbn=0u,mfd_lbn=0u;
    CHECK(port_dq_mfd_location(home,index,&mfd_vbn,&mfd_lbn)==PORT_DQ_MFD_OK);
    CHECK(at(f,mfd_lbn,mfd));
    const struct port_dq_dir_fid root={4u,4u,0u};
    CHECK(port_dq_dir_validate_header(mfd,&root)==PORT_DQ_DIR_OK);
    uint32_t dir_lbn=0u;
    CHECK(port_dq_dir_map_vbn(mfd,1u,&dir_lbn)==PORT_DQ_DIR_OK);
    CHECK(at(f,dir_lbn,mfddir));
    struct port_dq_dir_fid directory={0};
    CHECK(port_dq_dir_find(mfddir,"SYSEXE.DIR",10u,1u,&directory)==PORT_DQ_DIR_OK);
    CHECK(directory.number==10u&&directory.sequence==10u&&directory.volume==0u);
    uint32_t sysexe_vbn=(uint32_t)port_dq_index_word(home,32u)+
                         (uint32_t)port_dq_index_word(home,14u)*4u+
                         directory.number;
    uint32_t sysexe_lbn=0u;
    CHECK(port_dq_dir_map_vbn(index,sysexe_vbn,&sysexe_lbn)==PORT_DQ_DIR_OK);
    CHECK(at(f,sysexe_lbn,sysexe));
    CHECK(port_dq_dir_validate_header(sysexe,&directory)==PORT_DQ_DIR_OK);
    uint32_t sysdir_lbn=0u;
    CHECK(port_dq_dir_map_vbn(sysexe,1u,&sysdir_lbn)==PORT_DQ_DIR_OK);
    CHECK(at(f,sysdir_lbn,sysexedir));
    struct port_dq_dir_fid image={0};
    CHECK(port_dq_dir_find(sysexedir,"SYS.EXE",7u,1u,&image)==PORT_DQ_DIR_OK);
    CHECK(image.number==11u&&image.sequence==11u&&image.volume==0u);
    CHECK(port_dq_dir_find(sysexedir,"sys.exe",7u,1u,&image)==PORT_DQ_DIR_OK);
    CHECK(port_dq_dir_find(sysexedir,"SYS.EXE",7u,2u,&image)==PORT_DQ_DIR_NOT_FOUND);
    CHECK(port_dq_dir_find(sysexedir,"DOESNOT.EXE",11u,1u,&image)==PORT_DQ_DIR_NOT_FOUND);
    CHECK(port_dq_dir_find(NULL,"SYS.EXE",7u,1u,&image)==PORT_DQ_DIR_BAD);
    CHECK(port_dq_dir_find(sysexedir,NULL,7u,1u,&image)==PORT_DQ_DIR_BAD);
    CHECK(port_dq_dir_find(sysexedir,"SYS.EXE",0u,1u,&image)==PORT_DQ_DIR_BAD);
    CHECK(port_dq_dir_find(sysexedir,"SYS.EXE",7u,0u,&image)==PORT_DQ_DIR_BAD);
    CHECK(port_dq_dir_find(sysexedir,"SYS.EXE",7u,1u,NULL)==PORT_DQ_DIR_BAD);
    CHECK(port_dq_dir_validate_header(NULL,&directory)==PORT_DQ_DIR_BAD);
    CHECK(port_dq_dir_validate_header(sysexe,NULL)==PORT_DQ_DIR_BAD);
    const struct port_dq_dir_fid wrong={9u,10u,0u};
    CHECK(port_dq_dir_validate_header(sysexe,&wrong)==PORT_DQ_DIR_CORRUPT);
    uint8_t b[512];
    for(unsigned n=0u;n<7u;n++){
        memcpy(b,sysexe,512u);
        switch(n){
        case 0:b[7u]=1u;break;
        case 1:b[0u]=0u;break;
        case 2:b[1u]=b[0u];break;
        case 3:put16(b,8u,9u);break;
        case 4:put16(b,10u,9u);break;
        case 5:put16(b,12u,1u);break;
        default:b[510u]^=1u;break;
        }
        if(n!=6u)sum(b);
        CHECK(port_dq_dir_validate_header(b,&directory)==PORT_DQ_DIR_CORRUPT);
    }
    for(unsigned n=0u;n<6u;n++){
        memcpy(b,sysexe,512u);
        switch(n){
        case 0:b[7u]=1u;break;
        case 1:b[0u]=0u;break;
        case 2:b[1u]=b[0u];break;
        case 3:b[58u]=255u;break;
        case 4:b[510u]^=1u;break;
        default:put16(b,130u,UINT16_C(0xffff));break;
        }
        if(n!=4u)sum(b);
        uint32_t output=0xaabbccddu;
        CHECK(port_dq_dir_map_vbn(b,1u,&output)!=PORT_DQ_DIR_OK);
        CHECK(output==0xaabbccddu);
    }
    CHECK(port_dq_dir_map_vbn(NULL,1u,&sysdir_lbn)==PORT_DQ_DIR_BAD);
    CHECK(port_dq_dir_map_vbn(sysexe,0u,&sysdir_lbn)==PORT_DQ_DIR_BAD);
    CHECK(port_dq_dir_map_vbn(sysexe,1u,NULL)==PORT_DQ_DIR_BAD);
    CHECK(port_dq_dir_map_vbn(sysexe,UINT32_MAX,&sysdir_lbn)==PORT_DQ_DIR_MAP);
    for(unsigned n=0u;n<7u;n++){
        memcpy(b,sysexedir,512u);
        switch(n){
        case 0:put16(b,0u,0u);break;
        case 1:put16(b,0u,1u);break;
        case 2:put16(b,0u,UINT16_C(0xfffe));break;
        case 3:b[5u]=0u;break;
        case 4:b[5u]=255u;break;
        case 5:put16(b,0u,511u);break;
        default:put16(b,0u,509u);break;
        }
        const uint32_t st=port_dq_dir_find(b,"SYS.EXE",7u,1u,&image);
        CHECK(st==PORT_DQ_DIR_CORRUPT);
    }
    CHECK(fclose(f)==0);
    puts("FIL$FINDFILID real MFD -> SYSEXE.DIR -> SYS.EXE source directory traversal PASS");
    return 0;
}
