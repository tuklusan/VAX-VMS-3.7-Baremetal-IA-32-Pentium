#include "vms_f11_range.h"
/* Source FIL$READVBN/GETRTRVPTR uses one-origin VBNs; preflight all
 * required VBNs before any guest buffer is overwritten. Overflow and
 * missing extents fail without touching the caller's output. */
uint32_t port_f11_range_preflight(const uint8_t fh2[512],uint32_t offset,
    uint32_t count){
    if(!fh2||!count||
       (uint64_t)offset+count>UINT32_MAX)return PORT_F11_RANGE_BAD;
    const uint32_t first=offset/512u+1u;
    const uint32_t last=(uint32_t)(((uint64_t)offset+count-1u)/512u+1u);
    if(first>last||last-first>=UINT32_C(0x10000))return PORT_F11_RANGE_BAD;
    for(uint32_t vbn=first;vbn<=last;vbn++){
        uint32_t physical=0u;
        if(port_dq_dir_map_vbn(fh2,vbn,&physical)!=PORT_DQ_DIR_OK)
            return PORT_F11_RANGE_MAP;
    }
    return PORT_F11_RANGE_OK;
}
