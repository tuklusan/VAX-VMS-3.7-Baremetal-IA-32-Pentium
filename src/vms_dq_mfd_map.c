#include "vms_dq_mfd_header.h"
/* Exact FH2 compact retrieval pointer formats: format 1 (4 bytes),
 * format 2 (6 bytes), format 3 (8 bytes), and format 0 sparse skip.
 * VBN is one-origin. Bound every physical block to native LBA28. */
uint32_t port_dq_mfd_location(const uint8_t home[512],
       const uint8_t index_header[512],uint32_t *vbn,uint32_t *lbn){
    if(!home||!index_header||!vbn||!lbn)return PORT_DQ_MFD_BAD;
    uint32_t hdr_lbn=0u;
    if(port_dq_index_location(home,&hdr_lbn)!=PORT_DQ_INDEX_OK||
       index_header[7u]!=2u||port_dq_index_word(index_header,8u)!=1u||
       port_dq_index_word(index_header,10u)!=1u||
       port_dq_index_word(index_header,12u)!=0u||
       !port_dq_index_checksum(index_header,510u))return PORT_DQ_MFD_HEADER;
    const uint32_t cluster=port_dq_index_word(home,14u);
    const uint32_t bitmap=port_dq_index_word(home,32u);
    const uint64_t vn=(uint64_t)bitmap+4u*(uint64_t)cluster+4u;
    if(!vn||vn>UINT32_MAX)return PORT_DQ_MFD_MAP;
    const unsigned offset=(unsigned)index_header[1u]*2u;
    const unsigned size=(unsigned)index_header[58u]*2u;
    if(index_header[0u]==0u||index_header[1u]<=index_header[0u]||
       !size||offset>=510u||size>510u-offset)
        return PORT_DQ_MFD_MAP;
    uint32_t current=0u;
    for(unsigned p=offset;p<offset+size;){
        if(p+2u>offset+size)return PORT_DQ_MFD_MAP;
        const uint16_t first=port_dq_index_word(index_header,p);
        const unsigned format=first>>14;
        unsigned width=0u;
        uint32_t blocks=0u,base=0u;
        if(format==0u){width=2u;}
        else if(format==1u){
            width=4u;
            blocks=(first&255u)+1u;
            if(p+width>offset+size)return PORT_DQ_MFD_MAP;
            base=(((uint32_t)first>>8)&63u)<<16;
            base|=port_dq_index_word(index_header,p+2u);
        }else if(format==2u){
            width=6u;blocks=(first&UINT16_C(0x3fff))+1u;
            if(p+width>offset+size)return PORT_DQ_MFD_MAP;
            base=port_dq_index_long(index_header,p+2u);
        }else{
            width=8u;
            if(p+width>offset+size)return PORT_DQ_MFD_MAP;
            blocks=(((uint32_t)first&UINT16_C(0x3fff))<<16 |
                    port_dq_index_word(index_header,p+2u))+1u;
            base=port_dq_index_long(index_header,p+4u);
        }
        p+=width;
        if(!blocks)continue;
        if(base>=UINT32_C(0x10000000)||
           (uint64_t)base+blocks>UINT32_C(0x10000000)||
           (uint64_t)current+blocks>UINT32_MAX)
            return PORT_DQ_MFD_MAP;
        const uint32_t target=(uint32_t)vn-1u;
        if(target>=current&&target-current<blocks){
            *vbn=(uint32_t)vn;
            *lbn=base+(target-current);
            return PORT_DQ_MFD_OK;
        }
        current+=blocks;
    }
    return PORT_DQ_MFD_MAP;
}
