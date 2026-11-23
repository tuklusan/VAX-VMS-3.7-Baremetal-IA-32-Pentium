#include "vms_dq_aux_images.h"
/* Select the unique executable, file-backed PT_LOAD for a preloaded
 * processor/INIT image. Independent byte bounds are checked before any
 * disk read or privileged memory dereference. */
uint32_t port_dq_aux_descriptor(const uint8_t elf[512],
    uint32_t start,uint32_t end,struct port_dq_aux_segment *out){
    if(!elf||!out||start>=end||start<0x00320000u||end>0x00400000u)
        return PORT_DQ_AUX_BAD;
    if(elf[0]!=127u||elf[1]!='E'||elf[2]!='L'||elf[3]!='F'||
       elf[4]!=1u||elf[5]!=1u||elf[6]!=1u||
       port_dq_index_word(elf,16u)!=2u||port_dq_index_word(elf,18u)!=3u||
       port_dq_index_long(elf,20u)!=1u||
       port_dq_index_word(elf,40u)!=52u||
       port_dq_index_word(elf,42u)!=32u)return PORT_DQ_AUX_FORMAT;
    const uint32_t phoff=port_dq_index_long(elf,28u);
    const uint32_t phnum=port_dq_index_word(elf,44u);
    const uint32_t entry=port_dq_index_long(elf,24u);
    if(phnum<1u||phnum>10u||phoff<52u||
       (uint64_t)phoff+phnum*32u>512u||entry<start||entry>=end)
        return PORT_DQ_AUX_FORMAT;
    unsigned found=0u;
    struct port_dq_aux_segment result={0};
    for(uint32_t i=0u;i<phnum;i++){
        const uint32_t pos=phoff+32u*i;
        if(port_dq_index_long(elf,pos)!=1u)continue;
        const uint32_t offset=port_dq_index_long(elf,pos+4u);
        const uint32_t va=port_dq_index_long(elf,pos+8u);
        const uint32_t pa=port_dq_index_long(elf,pos+12u);
        const uint32_t filesz=port_dq_index_long(elf,pos+16u);
        const uint32_t memsz=port_dq_index_long(elf,pos+20u);
        const uint32_t flags=port_dq_index_long(elf,pos+24u);
        if(!filesz){
            if(memsz&&(va<start||va>=end||
                (uint64_t)va+memsz>end))return PORT_DQ_AUX_FORMAT;
            continue;
        }
        if((flags&7u)!=5u||offset%512u||va%512u||va!=pa||
           filesz>memsz||va<start||va>=end||
           (uint64_t)va+memsz>end||
           (uint64_t)offset+filesz>UINT32_MAX||found)
            return PORT_DQ_AUX_FORMAT;
        result=(struct port_dq_aux_segment){va,offset,filesz,memsz,
                                          entry,(filesz+511u)/512u};
        found=1u;
    }
    if(!found||entry<result.va||entry>=result.va+result.memsz||
       result.sectors==0u||result.sectors>1024u)
        return PORT_DQ_AUX_FORMAT;
    *out=result;
    return PORT_DQ_AUX_OK;
}
