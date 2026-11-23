#include "vms_dq_dir.h"
/* Native, no RB730 emulation. FH2 extent pointer formats 0..3 match
 * source FIL$READVBN / GETRTRVPTR. VBNs count from one. */
uint32_t port_dq_dir_map_vbn(const uint8_t b[512],uint32_t vbn,
                             uint32_t *lbn){
    if(!b||!vbn||!lbn)return PORT_DQ_DIR_BAD;
    if(b[7u]!=2u||!b[0u]||b[1u]<=b[0u]||b[1u]>=255u||
       !port_dq_index_checksum(b,510u))return PORT_DQ_DIR_CORRUPT;
    unsigned pos=(unsigned)b[1u]*2u;
    unsigned bytes=(unsigned)b[58u]*2u;
    if(pos>=510u||!bytes||bytes>510u-pos)return PORT_DQ_DIR_MAP;
    const unsigned end=pos+bytes;
    uint32_t covered=0u;
    while(pos<end){
        if(end-pos<2u)return PORT_DQ_DIR_MAP;
        uint16_t w=port_dq_index_word(b,pos);
        unsigned f=w>>14u,used=0u;
        uint32_t count=0u,at=0u;
        switch(f){
        case 0u:used=2u;break;
        case 1u:
            used=4u;
            if(end-pos<used)return PORT_DQ_DIR_MAP;
            count=(w&255u)+1u;
            at=((((uint32_t)w>>8u)&63u)<<16u)|port_dq_index_word(b,pos+2u);
            break;
        case 2u:
            used=6u;
            if(end-pos<used)return PORT_DQ_DIR_MAP;
            count=(w&UINT16_C(0x3fff))+1u;
            at=port_dq_index_long(b,pos+2u);
            break;
        default:
            used=8u;
            if(end-pos<used)return PORT_DQ_DIR_MAP;
            count=(((uint32_t)w&UINT16_C(0x3fff))<<16u |
                   port_dq_index_word(b,pos+2u))+1u;
            at=port_dq_index_long(b,pos+4u);
            break;
        }
        pos+=used;
        if(!count)continue;
        if(at>=UINT32_C(0x10000000)||
           (uint64_t)at+count>UINT32_C(0x10000000)||
           UINT32_MAX-covered<count)return PORT_DQ_DIR_MAP;
        if(vbn-1u>=covered && vbn-1u-covered<count){
            *lbn=at+(vbn-1u-covered);
            return PORT_DQ_DIR_OK;
        }
        covered+=count;
    }
    return PORT_DQ_DIR_MAP;
}
