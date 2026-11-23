#include "vms_dq_imageprobe.h"
/* Native IA-32 ELF32 only. The original VAX image activation machinery
 * is not silently replaced by checking this header. */
uint32_t port_dq_image_check_elf32(const uint8_t b[512],
                                   uint32_t *entry,uint16_t *segments){
    if(!b||!entry||!segments)return PORT_DQ_IMAGE_BAD;
    if(b[0]!=127u||b[1]!='E'||b[2]!='L'||b[3]!='F'||
       b[4]!=1u||b[5]!=1u||b[6]!=1u||
       port_dq_index_word(b,16u)!=2u||  /* ET_EXEC */
       port_dq_index_word(b,18u)!=3u||  /* EM_386 */
       port_dq_index_long(b,20u)!=1u||
       port_dq_index_word(b,40u)!=52u||
       port_dq_index_word(b,42u)!=32u||
       port_dq_index_word(b,44u)==0u||
       port_dq_index_word(b,44u)>10u)return PORT_DQ_IMAGE_FORMAT;
    const uint32_t phoff=port_dq_index_long(b,28u);
    const uint16_t phnum=port_dq_index_word(b,44u);
    const uint32_t pc=port_dq_index_long(b,24u);
    if(phoff<52u||phoff>512u||
       (uint64_t)phoff+32u*(uint64_t)phnum>512u||
       pc<UINT32_C(0x00320000)||pc>=UINT32_C(0x00400000))
        return PORT_DQ_IMAGE_FORMAT;
    unsigned load=0u;
    for(uint16_t i=0u;i<phnum;i++){
        const uint32_t at=phoff+32u*(uint32_t)i;
        const uint32_t type=port_dq_index_long(b,at);
        if(type==1u){
            const uint32_t filesz=port_dq_index_long(b,at+16u);
            const uint32_t memsz=port_dq_index_long(b,at+20u);
            const uint32_t va=port_dq_index_long(b,at+8u);
            if(!filesz||memsz<filesz||va<UINT32_C(0x00320000)||
               va>=UINT32_C(0x00400000)||
               (uint64_t)va+memsz>UINT32_C(0x00400000))
                return PORT_DQ_IMAGE_FORMAT;
            load++;
        }
    }
    if(!load)return PORT_DQ_IMAGE_FORMAT;
    *entry=pc;*segments=(uint16_t)load;
    return PORT_DQ_IMAGE_OK;
}
