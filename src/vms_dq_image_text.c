#include "vms_dq_imageprobe.h"
/* Original FIL$READVBN / GETRTRVPTR: translate the first executable ELF
 * PT_LOAD byte offset to a Files-11 virtual block, then to a physical LBN.
 * This is disk-to-resident verification, not a VAX image activator. */
uint32_t port_dq_image_text_lbn(const uint8_t hdr[512],const uint8_t elf[512],
    uint32_t *lbn,uint32_t *va,uint32_t *offset){
    if(!hdr||!elf||!lbn||!va||!offset)return PORT_DQ_IMAGE_BAD;
    uint32_t entry=0u;uint16_t count=0u;
    if(port_dq_image_check_elf32(elf,&entry,&count)!=PORT_DQ_IMAGE_OK)
        return PORT_DQ_IMAGE_FORMAT;
    const uint32_t phoff=port_dq_index_long(elf,28u);
    const uint16_t phnum=port_dq_index_word(elf,44u);
    for(uint16_t i=0u;i<phnum;i++){
        const uint32_t pos=phoff+32u*(uint32_t)i;
        if(port_dq_index_long(elf,pos)!=1u)continue;
        const uint32_t off=port_dq_index_long(elf,pos+4u);
        const uint32_t v=port_dq_index_long(elf,pos+8u);
        const uint32_t pa=port_dq_index_long(elf,pos+12u);
        const uint32_t filesz=port_dq_index_long(elf,pos+16u);
        const uint32_t flags=port_dq_index_long(elf,pos+24u);
        if(!(flags&1u))continue;
        if((flags&2u)||off%512u||v%512u||v!=pa||filesz<512u||
           v<UINT32_C(0x00320000)||
           (uint64_t)v+filesz>UINT32_C(0x00400000)||
           (uint64_t)off/512u+1u>UINT32_MAX)
            return PORT_DQ_IMAGE_FORMAT;
        uint32_t physical=0u;
        if(port_dq_dir_map_vbn(hdr,off/512u+1u,&physical)!=PORT_DQ_DIR_OK)
            return PORT_DQ_IMAGE_MAP;
        *lbn=physical;*va=v;*offset=off;
        return PORT_DQ_IMAGE_OK;
    }
    return PORT_DQ_IMAGE_FORMAT;
}
