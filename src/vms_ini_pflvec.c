#include "vms_ini_pflvec.h"
#include <stdint.h>
static void word(uint8_t *p,uint16_t v){p[0]=(uint8_t)v;p[1]=(uint8_t)(v>>8);}
static void dword(uint8_t *p,uint32_t v){
    for(unsigned i=0;i<4u;i++)p[i]=(uint8_t)(v>>(8u*i));
}
static uint32_t read32(const uint8_t *p){
    return (uint32_t)p[0]|((uint32_t)p[1]<<8)|
           ((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);
}
uint32_t port_ini_pflvec_init(void *memory,size_t capacity,
                 const struct port_ini_pflvec_settings *s,
                 struct port_ini_pflvec *out){
    if(!memory||!s||!out||((uintptr_t)memory&3u)||
       !s->null_pfl_address || (s->null_pfl_address&3u) ||
       s->page_file_count>UINT16_MAX || s->swap_file_count>=UINT16_MAX ||
       s->ptr_dyn_type>UINT8_MAX || s->pfl_dyn_type>UINT8_MAX)
        return PORT_INI_PFL_BADPARAM;
    uint64_t n=(uint64_t)s->page_file_count+s->swap_file_count;
    uint64_t size=12u+4u*n;
    if(!n || size>UINT16_MAX || n>UINT16_MAX)
        return PORT_INI_PFL_BADPARAM;
    if(size>capacity)return PORT_INI_PFL_NOMEM;
    /* INIT_MISC computes shell's starting VBN from page-aligned address:
     * (SWP$GL_SHELLBAS & 0x7fffffff)/512 + 1 + 1.
     * Reject a misaligned region and 32-bit overflow, not emulate the VAX
     * addressing model. The native virtual address is passed explicitly. */
    /* The first native bring-up has no shell process or SYSWCB yet.
     * Do not invent a shell VBN or a pagefile window. The vector may be
     * constructed earlier, and the pagefile service remains blocked until
     * both original shell inputs exist. Reject mismatched partial inputs. */
    uint64_t vbn=0u;
    if(s->shell_base_address || s->shell_system_window){
        if(!s->shell_base_address || !s->shell_system_window)
            return PORT_INI_PFL_BADPARAM;
        uint32_t shell=s->shell_base_address&UINT32_C(0x7fffffff);
        if(shell&511u)return PORT_INI_PFL_BADPARAM;
        vbn=((uint64_t)shell>>9)+2u;
        if(vbn>UINT32_MAX)return PORT_INI_PFL_BADPARAM;
    }
    uint8_t *p=memory;
    for(uint32_t i=0;i<(uint32_t)size;i++)p[i]=0u;
    dword(p+4u,(uint32_t)n); /* PTR$L_PTRCNT */
    word(p+8u,(uint16_t)size); /* PTR$W_SIZE */
    p[10u]=(uint8_t)s->ptr_dyn_type;
    p[11u]=(uint8_t)s->pfl_dyn_type;
    for(uint32_t i=0;i<(uint32_t)n;i++)dword(p+12u+i*4u,s->null_pfl_address);
    *out=(struct port_ini_pflvec){
        .pointers=p+12u,.entries=(uint32_t)n,.allocated_bytes=(uint32_t)size,
        .null_pagefile_vbn=(uint32_t)vbn,
        .null_pagefile_window=s->shell_system_window,
        .actual_swapfile_slots=s->swap_file_count+1u,
        .global_page_file_limit=s->global_page_file_limit};
    return PORT_INI_PFL_OK;
}
uint32_t port_ini_pflvec_entry(const struct port_ini_pflvec *s,uint32_t i){
    if(!s||!s->pointers||i>=s->entries)return 0u;
    return read32(s->pointers+4u*i);
}
uint32_t port_ini_interrupt_stack_limits(uint32_t high,uint32_t pages,
                    uint32_t extra,struct port_ini_interrupt_stack *out){
    if(!out||!pages||pages>UINT16_MAX)return PORT_INI_PFL_BADPARAM;
    uint64_t bytes=(uint64_t)pages*512u;
    if(bytes>high)return PORT_INI_PFL_BADPARAM;
    uint32_t room=extra>=bytes?0u:(uint32_t)(bytes-extra);
    uint32_t depth=room/16u;
    if(depth<4u)depth=4u;
    if(depth>255u)depth=255u;
    *out=(struct port_ini_interrupt_stack){.low=high-(uint32_t)bytes,
            .high=high,.max_resource_depth=(uint8_t)depth};
    return PORT_INI_PFL_OK;
}
