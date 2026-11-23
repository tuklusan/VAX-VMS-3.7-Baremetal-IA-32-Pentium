#include "vms_f11_chain.h"
/* One-origin VBN to LBN across multiple separately checksummed FH2 records. */
uint32_t port_f11_chain_lbn(const struct port_f11_chain *chain,
                           uint32_t vbn,uint32_t *lbn){
    if(!chain||!vbn||!lbn||!chain->count||
       chain->count>PORT_F11_CHAIN_MAX_EXTENTS)
        return PORT_F11_CHAIN_BAD;
    uint64_t start=0u;
    for(uint32_t i=0u;i<chain->count;i++){
        const struct port_f11_extent *e=&chain->extents[i];
        if(!e->blocks||!e->lbn||
           (uint64_t)e->lbn+e->blocks>UINT32_C(0x10000000))
            return PORT_F11_CHAIN_MAP;
        const uint64_t end=start+e->blocks;
        if((uint64_t)vbn-1u>=start&&(uint64_t)vbn-1u<end){
            *lbn=e->lbn+(uint32_t)((uint64_t)vbn-1u-start);
            return PORT_F11_CHAIN_OK;
        }
        start=end;
        if(start>UINT32_MAX)return PORT_F11_CHAIN_MAP;
    }
    return PORT_F11_CHAIN_MAP;
}
