#include "vms_dq_imageprobe.h"
/* Source FILEREAD.MAR FIL$READVBN, GETRTRVPTR: one sector of the
 * executable PT_LOAD extent, using the actual file header retrieval map.
 * Every call revalidates the original ELF program-header descriptor. */
uint32_t port_dq_image_text_sector(const uint8_t header[512],
    const uint8_t elf[512],uint32_t sector_index,
    uint32_t *lbn,uint32_t *va,uint32_t *valid_bytes){
    if(!header||!elf||!lbn||!va||!valid_bytes)return PORT_DQ_IMAGE_BAD;
    uint32_t first_lbn=0u,first_va=0u,file_off=0u;
    uint32_t rc=port_dq_image_text_lbn(header,elf,&first_lbn,&first_va,&file_off);
    if(rc!=PORT_DQ_IMAGE_OK)return rc;
    uint32_t filesz=0u;
    if(port_dq_image_text_filesz(elf,first_va,file_off,&filesz)!=PORT_DQ_IMAGE_OK)
        return PORT_DQ_IMAGE_FORMAT;
    {
        const uint64_t delta=(uint64_t)sector_index*512u;
        const uint64_t address=(uint64_t)first_va+delta;
        const uint64_t file_byte=(uint64_t)file_off+delta;
        if(delta>=filesz||address>=UINT32_C(0x00400000)||
           file_byte/512u+1u>UINT32_MAX)return PORT_DQ_IMAGE_MAP;
        uint32_t physical=0u;
        if(port_dq_dir_map_vbn(header,(uint32_t)(file_byte/512u+1u),
                              &physical)!=PORT_DQ_DIR_OK)return PORT_DQ_IMAGE_MAP;
        *lbn=physical;*va=(uint32_t)address;
        *valid_bytes=filesz-delta>=512u?512u:(uint32_t)(filesz-delta);
        return PORT_DQ_IMAGE_OK;
    }
    return PORT_DQ_IMAGE_FORMAT;
}
uint32_t port_dq_image_text_filesz(const uint8_t elf[512],
    uint32_t first_va,uint32_t file_off,uint32_t *filesz){
    if(!elf||!filesz)return PORT_DQ_IMAGE_BAD;
    uint32_t entry=0u;uint16_t segments=0u;
    if(port_dq_image_check_elf32(elf,&entry,&segments)!=PORT_DQ_IMAGE_OK)
        return PORT_DQ_IMAGE_FORMAT;
    const uint32_t phoff=port_dq_index_long(elf,28u);
    const uint16_t count=port_dq_index_word(elf,44u);
    for(uint16_t i=0u;i<count;i++){
        const uint32_t pos=phoff+32u*(uint32_t)i;
        if(port_dq_index_long(elf,pos)==1u&&
           port_dq_index_long(elf,pos+8u)==first_va&&
           port_dq_index_long(elf,pos+4u)==file_off&&
           (port_dq_index_long(elf,pos+24u)&1u)&&
           !(port_dq_index_long(elf,pos+24u)&2u)){
            uint32_t n=port_dq_index_long(elf,pos+16u);
            if(n<512u||(uint64_t)first_va+n>UINT32_C(0x00400000))
                return PORT_DQ_IMAGE_FORMAT;
            *filesz=n;return PORT_DQ_IMAGE_OK;
        }
    }
    return PORT_DQ_IMAGE_FORMAT;
}
