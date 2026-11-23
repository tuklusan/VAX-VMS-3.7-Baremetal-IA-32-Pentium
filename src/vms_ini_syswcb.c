#include "vms_ini_syswcb.h"
#include <stdint.h>
static uint16_t read16(const uint8_t *p){
    return (uint16_t)p[0] | (uint16_t)((uint16_t)p[1]<<8);
}
static uint32_t read32(const uint8_t *p){
    return (uint32_t)p[0] | ((uint32_t)p[1]<<8) |
           ((uint32_t)p[2]<<16) | ((uint32_t)p[3]<<24);
}
static void write16(uint8_t *p,uint16_t v){
    p[0]=(uint8_t)v;p[1]=(uint8_t)(v>>8);
}
static void write32(uint8_t *p,uint32_t v){
    for(uint32_t i=0;i<4u;i++)p[i]=(uint8_t)(v>>(8u*i));
}
uint32_t port_ini_syswcb_from_boot_map(
    uint8_t *boot_map,size_t capacity,
    const uint8_t *template_data,size_t template_capacity,
    const struct port_ini_syswcb_layout *layout,
    struct port_ini_syswcb *result){
    if(!boot_map||!template_data||!layout||!result||
       capacity<4u||layout->header_length<2u||
       layout->header_length>template_capacity||
       layout->header_length>UINT16_MAX||
       layout->size_word_offset>layout->header_length-2u||
       layout->map_count_word_offset>layout->header_length-2u)
        return PORT_INI_SYSWCB_BADPARAM;
    uint32_t a=layout->size_word_offset,b=layout->map_count_word_offset;
    if((a>b?a-b:b-a)<2u)return PORT_INI_SYSWCB_BADPARAM;
    /* The WCB template is the prelinked resident constant. It must not
     * be within the writable boot-map span, even if the caller passed it
     * as an overlapping pointer with a different object type. */
    uintptr_t map=(uintptr_t)boot_map,t=(uintptr_t)template_data;
    if((map<=t && t-map<capacity) ||
       (t<=map && map-t<layout->header_length))
        return PORT_INI_SYSWCB_BADPARAM;
    uint32_t normalized_bytes=read32(boot_map);
    if(!normalized_bytes || (normalized_bytes&7u))
        return PORT_INI_SYSWCB_BADMAP;
    uint32_t count=normalized_bytes>>3;
    if(count>UINT16_MAX || (uint64_t)normalized_bytes+4u>capacity)
        return PORT_INI_SYSWCB_BADMAP;
    uint64_t output_bytes=(uint64_t)layout->header_length+6u*count;
    if(output_bytes>capacity || output_bytes>UINT32_MAX)
        return PORT_INI_SYSWCB_NOMEM;
    uint32_t original_size=read16(template_data+a);
    if(!original_size || original_size+6u*count>UINT16_MAX)
        return PORT_INI_SYSWCB_BADMAP;
    /* Validate all extents before rewriting a byte. INIT says SYS.EXE
     * cannot need 65536 blocks in a single 16-bit retrieval pointer. */
    for(uint32_t i=0;i<count;i++){
        uint32_t n=read32(boot_map+4u+i*8u);
        uint32_t lbn=read32(boot_map+8u+i*8u);
        if(!n||n>UINT16_MAX || lbn>UINT32_MAX-n)
            return PORT_INI_SYSWCB_BADMAP;
    }
    for(uint32_t i=0;i<count;i++){
        const uint8_t *from=boot_map+4u+i*8u;
        uint16_t blocks=(uint16_t)read32(from);
        uint32_t lbn=read32(from+4u);
        uint8_t *to=boot_map+6u*i;
        write16(to,blocks);
        write32(to+2u,lbn);
    }
    /* Freestanding IA-32 kernel: no libc calls or VAX MOVC3 emulation.
     * Move from the end because the destination can overlap the source. */
    for(uint32_t n=6u*count;n>0u;n--)
        boot_map[layout->header_length+n-1u]=boot_map[n-1u];
    for(uint32_t n=0u;n<layout->header_length;n++)
        boot_map[n]=template_data[n];
    write16(boot_map+a,(uint16_t)(original_size+6u*count));
    write16(boot_map+b,(uint16_t)count);
    *result=(struct port_ini_syswcb){
        .record=boot_map,
        .retrieval_pointers=boot_map+layout->header_length,
        .mapping_count=(uint16_t)count,
        .record_size_field=(uint16_t)(original_size+6u*count),
        .record_length=(uint32_t)output_bytes};
    return PORT_INI_SYSWCB_OK;
}
uint32_t port_ini_syswcb_retrieval(const struct port_ini_syswcb *wcb,
        uint32_t index,uint16_t *blocks,uint32_t *starting_lbn){
    if(!wcb||!wcb->retrieval_pointers||!blocks||!starting_lbn||
       index>=wcb->mapping_count)return PORT_INI_SYSWCB_BADPARAM;
    const uint8_t *p=wcb->retrieval_pointers+6u*index;
    *blocks=read16(p);*starting_lbn=read32(p+2u);
    return PORT_INI_SYSWCB_OK;
}
