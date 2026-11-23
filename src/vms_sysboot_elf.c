#include "vms_sysboot_elf.h"
#include "vms_sysboot_read.h"
/* No host libc in the bootstrap. ELF is little-endian, 32-bit and static. */
static uint16_t rd16(const uint8_t *p){return (uint16_t)((uint16_t)p[0]|((uint16_t)p[1]<<8));}
static uint32_t rd32(const uint8_t *p){return (uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);}
struct load { uint32_t offset,physical,filesz,memsz,flags; };
static uint32_t read_bytes(const struct boo_io_driver *disk,uint32_t fn,
                           const uint8_t fh[512],uint32_t at,
                           uint8_t *out,uint32_t size){
    uint8_t sector[512];
    while(size){
        uint32_t off=at&511u;
        uint32_t n=512u-off;
        if(n>size)n=size;
        if(port_sysboot_read_virtual(disk,fn,fh,at/512u+1u,512u,
                                     sector,sizeof sector)!=PORT_F11_HOME_OK)
            return PORT_SYS_ELF_DISK_ERROR;
        for(uint32_t j=0;j<n;j++)out[j]=sector[off+j];
        at+=n;out+=n;size-=n;
    }
    return PORT_SYS_ELF_OK;
}
uint32_t port_sysboot_load_elf_range(const struct boo_io_driver *disk,uint32_t fn,
                               const uint8_t fh[512],uint32_t blocks,
                               const struct port_sys_elf_memory *mem,
                               uint32_t first_address,uint32_t limit_address,
                               uint32_t *entry_out){
    if(!disk||!fh||!mem||!mem->write||!mem->clear||!entry_out||!blocks||
       blocks>UINT32_MAX/512u||first_address>=limit_address||
       first_address<PORT_SYS_ELF_START||
       limit_address>PORT_SYSLOA386_ELF_LIMIT)return PORT_SYS_ELF_BAD_ARG;
    uint8_t header[512];
    if(read_bytes(disk,fn,fh,0u,header,512u)!=PORT_SYS_ELF_OK)
        return PORT_SYS_ELF_DISK_ERROR;
    if(header[0]!=0x7fu||header[1]!='E'||header[2]!='L'||header[3]!='F'||
       header[4]!=1u||header[5]!=1u||header[6]!=1u||
       rd16(header+16u)!=2u||rd16(header+18u)!=3u||
       rd32(header+20u)!=1u||rd16(header+40u)!=52u||
       rd16(header+42u)!=32u)return PORT_SYS_ELF_BAD_HEADER;
    uint32_t phoff=rd32(header+28u),entry=rd32(header+24u);
    uint16_t count=rd16(header+44u);
    if(!count||count>8u||phoff<52u||phoff>512u-
       (uint32_t)count*32u)return PORT_SYS_ELF_BAD_HEADER;
    struct load seg[4];uint32_t used=0u;int entry_ok=0;
    for(uint16_t i=0;i<count;i++){
        const uint8_t *ph=header+phoff+(uint32_t)i*32u;
        if(rd32(ph)!=1u)continue;
        struct load s={rd32(ph+4u),rd32(ph+12u),rd32(ph+16u),
                       rd32(ph+20u),rd32(ph+24u)};
        /* ELF permits a PT_LOAD with no file or memory footprint. */
        if(!s.memsz && !s.filesz)continue;
        if(used==4u)return PORT_SYS_ELF_BAD_SEGMENT;
        uint32_t virtual_address=rd32(ph+8u);
        if(!s.memsz||s.filesz>s.memsz||s.physical!=virtual_address||
           s.physical<first_address||
           s.physical>=limit_address||
           s.memsz>limit_address-s.physical||
           s.offset>blocks*512u||s.filesz>blocks*512u-s.offset||
           !(s.flags&4u)||(s.flags&2u && s.flags&1u))
            return PORT_SYS_ELF_BAD_SEGMENT;
        for(uint32_t j=0;j<used;j++){
            uint32_t a=seg[j].physical,b=seg[j].memsz;
            if(s.physical<a+b&&a<s.physical+s.memsz)
                return PORT_SYS_ELF_BAD_SEGMENT;
        }
        if((s.flags&1u)&&entry>=s.physical&&entry-s.physical<s.memsz)
            entry_ok=1;
        seg[used++]=s;
    }
    if(!used||!entry_ok)return PORT_SYS_ELF_BAD_SEGMENT;
    /* Preflight all headers and intervals before touching executable RAM. */
    for(uint32_t i=0;i<used;i++){
        struct load *s=&seg[i];
        uint32_t at=0;
        while(at<s->filesz){
            uint8_t bytes[512];
            uint32_t n=s->filesz-at;
            if(n>sizeof bytes)n=sizeof bytes;
            if(read_bytes(disk,fn,fh,s->offset+at,bytes,n)!=PORT_SYS_ELF_OK)
                return PORT_SYS_ELF_DISK_ERROR;
            if(!mem->write(mem->context,s->physical+at,bytes,n))
                return PORT_SYS_ELF_WRITE_ERROR;
            at+=n;
        }
        if(s->memsz>s->filesz &&
           !mem->clear(mem->context,s->physical+s->filesz,
                       s->memsz-s->filesz))return PORT_SYS_ELF_WRITE_ERROR;
    }
    *entry_out=entry;
    return PORT_SYS_ELF_OK;
}

uint32_t port_sysboot_load_elf(const struct boo_io_driver *disk,uint32_t fn,
                               const uint8_t fh[512],uint32_t blocks,
                               const struct port_sys_elf_memory *mem,
                               uint32_t *entry_out){
    return port_sysboot_load_elf_range(disk,fn,fh,blocks,mem,
              PORT_SYS_ELF_START,PORT_SYS_ELF_LIMIT,entry_out);
}
