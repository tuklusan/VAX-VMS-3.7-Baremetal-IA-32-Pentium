#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "vms_dq_imageprobe.h"
#define CHECK(x) do{if(!(x)){fprintf(stderr,"SYS.EXE native image test %d: %s\n",__LINE__,#x);return 1;}}while(0)
static int at(FILE *f,uint32_t lbn,uint8_t b[512]){
    return fseek(f,(long)lbn*512L,SEEK_SET)==0&&fread(b,1,512u,f)==512u;
}
static void set16(uint8_t *b,unsigned pos,uint16_t x){b[pos]=(uint8_t)x;b[pos+1u]=(uint8_t)(x>>8u);}
static void set32(uint8_t *b,unsigned pos,uint32_t x){set16(b,pos,(uint16_t)x);set16(b,pos+2u,(uint16_t)(x>>16u));}
static void sum(uint8_t b[512]){uint16_t s=0;for(unsigned i=0;i<510u;i+=2u)s=(uint16_t)(s+port_dq_index_word(b,i));set16(b,510u,s);}
int main(void){
    FILE *f=fopen("portdisk.img","rb");CHECK(f);
    uint8_t home[512],index[512],sysfile[512],elf[512],b[512];
    CHECK(at(f,1u,home));
    uint32_t index_lbn=0u;
    CHECK(port_dq_index_location(home,&index_lbn)==PORT_DQ_INDEX_OK);
    CHECK(at(f,index_lbn,index));
    const struct port_dq_dir_fid id={11u,11u,0u};
    uint32_t sysheader_lbn=0u,sysdata_lbn=0u;
    CHECK(port_dq_image_header_lbn(home,index,&id,&sysheader_lbn)==PORT_DQ_IMAGE_OK);
    CHECK(at(f,sysheader_lbn,sysfile));
    CHECK(port_dq_image_verify_fh2(sysfile,&id)==PORT_DQ_IMAGE_OK);
    CHECK(port_dq_image_first_data(sysfile,&sysdata_lbn)==PORT_DQ_IMAGE_OK);
    CHECK(at(f,sysdata_lbn,elf));
    uint32_t entry=0u;uint16_t segments=0u;
    CHECK(port_dq_image_check_elf32(elf,&entry,&segments)==PORT_DQ_IMAGE_OK);
    CHECK(entry>=0x00320000u&&entry<0x00400000u&&segments>=1u&&segments<=10u);
    uint32_t text_lbn=0u,text_va=0u,text_offset=0u;
    CHECK(port_dq_image_text_lbn(sysfile,elf,&text_lbn,&text_va,&text_offset)==PORT_DQ_IMAGE_OK);
    CHECK(text_va==0x00320000u&&text_offset==4096u);
    uint8_t disk_text[512],native_text[512];
    CHECK(at(f,text_lbn,disk_text));
    FILE *native=fopen("build/sys.exe","rb");CHECK(native);
    CHECK(fseek(native,(long)text_offset,SEEK_SET)==0);
    CHECK(fread(native_text,1,512u,native)==512u);
    CHECK(memcmp(disk_text,native_text,512u)==0);
    uint32_t text_length=0u;
    CHECK(port_dq_image_text_filesz(elf,text_va,text_offset,&text_length)==PORT_DQ_IMAGE_OK);
    CHECK(text_length>=8192u&&text_length<UINT32_C(0x20000));
    const uint32_t sectors=(text_length+511u)/512u;
    uint32_t compared=0u;
    for(uint32_t n=0u;n<sectors;n++){
        uint32_t sector_lbn=0u,sector_va=0u,valid_bytes=0u;
        CHECK(port_dq_image_text_sector(sysfile,elf,n,&sector_lbn,&sector_va,
                 &valid_bytes)==PORT_DQ_IMAGE_OK);
        CHECK(sector_va==text_va+512u*n);
        CHECK(valid_bytes==((n+1u)*512u>text_length?text_length-n*512u:512u));
        CHECK(at(f,sector_lbn,disk_text));
        CHECK(fseek(native,(long)(text_offset+512u*n),SEEK_SET)==0);
        CHECK(fread(native_text,1,valid_bytes,native)==valid_bytes);
        CHECK(memcmp(disk_text,native_text,valid_bytes)==0);
        compared+=valid_bytes;
    }
    CHECK(compared==text_length);
    for(uint32_t n=0u;n<6u;n++){
        uint32_t sector_lbn=0xdecafbad,sector_va=0xdecafbad,bytes=0xdecafbad;
        CHECK(port_dq_image_text_sector(sysfile,elf,UINT32_MAX-n,&sector_lbn,&sector_va,&bytes)!=PORT_DQ_IMAGE_OK);
        CHECK(sector_lbn==0xdecafbad&&sector_va==0xdecafbad&&bytes==0xdecafbad);
    }
    uint32_t bytes=0u;
    CHECK(port_dq_image_text_sector(sysfile,elf,0u,NULL,&text_va,&bytes)==PORT_DQ_IMAGE_BAD);
    CHECK(port_dq_image_text_sector(sysfile,elf,0u,&text_lbn,NULL,&bytes)==PORT_DQ_IMAGE_BAD);
    CHECK(port_dq_image_text_sector(NULL,elf,0u,&text_lbn,&text_va,&bytes)==PORT_DQ_IMAGE_BAD);
    CHECK(port_dq_image_text_sector(sysfile,NULL,0u,&text_lbn,&text_va,&bytes)==PORT_DQ_IMAGE_BAD);
    CHECK(port_dq_image_text_sector(sysfile,elf,0u,&text_lbn,&text_va,NULL)==PORT_DQ_IMAGE_BAD);
    CHECK(port_dq_image_text_sector(sysfile,elf,sectors,&text_lbn,&text_va,&bytes)==PORT_DQ_IMAGE_MAP);
    CHECK(fclose(native)==0);
    CHECK(port_dq_image_text_lbn(NULL,elf,&text_lbn,&text_va,&text_offset)==PORT_DQ_IMAGE_BAD);
    CHECK(port_dq_image_text_lbn(sysfile,NULL,&text_lbn,&text_va,&text_offset)==PORT_DQ_IMAGE_BAD);
    CHECK(port_dq_image_text_lbn(sysfile,elf,NULL,&text_va,&text_offset)==PORT_DQ_IMAGE_BAD);
    CHECK(port_dq_image_text_lbn(sysfile,elf,&text_lbn,NULL,&text_offset)==PORT_DQ_IMAGE_BAD);
    CHECK(port_dq_image_text_lbn(sysfile,elf,&text_lbn,&text_va,NULL)==PORT_DQ_IMAGE_BAD);
    for(unsigned i=0;i<24u;i++){
        memcpy(b,elf,512u);
        /* Mutation cases cover executable flags, load offsets, virtual
         * addresses, range/size and competing load descriptors. */
        if(i<4u)set32(b,52u+24u,(uint32_t)(i*2u));
        else if(i<8u)set32(b,52u+4u,(uint32_t)(4097u+i));
        else if(i<12u)set32(b,52u+8u,UINT32_C(0x00320001)+i);
        else if(i<16u)set32(b,52u+12u,UINT32_C(0x00320010)+i);
        else if(i<20u)set32(b,52u+16u,(uint32_t)(i-16u));
        else set32(b,52u+4u,UINT32_MAX-512u+(i-20u));
        CHECK(port_dq_image_text_lbn(sysfile,b,&text_lbn,&text_va,&text_offset)!=PORT_DQ_IMAGE_OK);
    }
    CHECK(port_dq_image_header_lbn(NULL,index,&id,&sysheader_lbn)==PORT_DQ_IMAGE_BAD);
    CHECK(port_dq_image_header_lbn(home,NULL,&id,&sysheader_lbn)==PORT_DQ_IMAGE_BAD);
    CHECK(port_dq_image_header_lbn(home,index,NULL,&sysheader_lbn)==PORT_DQ_IMAGE_BAD);
    CHECK(port_dq_image_header_lbn(home,index,&id,NULL)==PORT_DQ_IMAGE_BAD);
    const struct port_dq_dir_fid invalid={0u,0u,0u};
    CHECK(port_dq_image_header_lbn(home,index,&invalid,&sysheader_lbn)==PORT_DQ_IMAGE_BAD);
    CHECK(port_dq_image_verify_fh2(NULL,&id)==PORT_DQ_IMAGE_BAD);
    CHECK(port_dq_image_verify_fh2(sysfile,NULL)==PORT_DQ_IMAGE_BAD);
    CHECK(port_dq_image_verify_fh2(sysfile,&invalid)==PORT_DQ_IMAGE_BAD);
    CHECK(port_dq_image_first_data(NULL,&sysdata_lbn)==PORT_DQ_IMAGE_BAD);
    CHECK(port_dq_image_first_data(sysfile,NULL)==PORT_DQ_IMAGE_BAD);
    CHECK(port_dq_image_check_elf32(NULL,&entry,&segments)==PORT_DQ_IMAGE_BAD);
    CHECK(port_dq_image_check_elf32(elf,NULL,&segments)==PORT_DQ_IMAGE_BAD);
    CHECK(port_dq_image_check_elf32(elf,&entry,NULL)==PORT_DQ_IMAGE_BAD);
    for(unsigned n=0;n<8u;n++){
        memcpy(b,sysfile,512u);
        switch(n){
        case 0:b[7u]=1u;break;
        case 1:b[0u]=0u;break;
        case 2:b[1u]=b[0u];break;
        case 3:b[1u]=255u;break;
        case 4:set16(b,8u,9u);break;
        case 5:set16(b,10u,9u);break;
        case 6:set16(b,12u,1u);break;
        default:b[510u]^=1u;break;
        }
        if(n!=7u)sum(b);
        CHECK(port_dq_image_verify_fh2(b,&id)==PORT_DQ_IMAGE_HEADER);
    }
    for(unsigned n=0;n<21u;n++){
        memcpy(b,elf,512u);
        switch(n){
        case 0:b[0u]=0u;break;
        case 1:b[1u]=0u;break;
        case 2:b[2u]=0u;break;
        case 3:b[3u]=0u;break;
        case 4:b[4u]=2u;break;
        case 5:b[5u]=2u;break;
        case 6:b[6u]=0u;break;
        case 7:set16(b,16u,3u);break;
        case 8:set16(b,18u,8u);break;
        case 9:set32(b,20u,0u);break;
        case 10:set32(b,24u,0u);break;
        case 11:set32(b,24u,UINT32_MAX);break;
        case 12:set32(b,28u,500u);break;
        case 13:set16(b,40u,0u);break;
        case 14:set16(b,42u,16u);break;
        case 15:set16(b,44u,0u);break;
        case 16:set16(b,44u,11u);break;
        case 17:set32(b,52u,0u);set32(b,84u,0u);break;
        case 18:set32(b,52u+8u,UINT32_C(0x00400000));break;
        case 19:set32(b,52u+20u,1u);break;
        default:set32(b,52u+16u,0u);break;
        }
        uint32_t pc=UINT32_C(0xdecafbad);uint16_t nseg=0xbeefu;
        CHECK(port_dq_image_check_elf32(b,&pc,&nseg)==PORT_DQ_IMAGE_FORMAT);
        CHECK(pc==UINT32_C(0xdecafbad)&&nseg==0xbeefu);
    }
    CHECK(fclose(f)==0);
    puts("Source FIL$READVBN SYS.EXE FH2 and native IA-32 ELF32 real first block PASS");
    return 0;
}
