#include "vms_dq_aux_images.h"
/* Source FIL$READVBN GETRTRVPTR physical sector calculation. Output is
 * atomic: malformed FH2/VBN never leaves a half-valid transfer request. */
uint32_t port_dq_aux_sector(const uint8_t fh2[512],
    const struct port_dq_aux_segment *seg,uint32_t index,
    uint32_t *lbn,uint32_t *va,uint32_t *valid){
    if(!fh2||!seg||!lbn||!va||!valid)return PORT_DQ_AUX_BAD;
    if(!seg->filesz||!seg->sectors||
       (uint64_t)seg->filesz+511u>UINT32_MAX||
       seg->sectors!=(seg->filesz+511u)/512u||
       index>=seg->sectors||seg->offset%512u||seg->va%512u)
        return PORT_DQ_AUX_FORMAT;
    const uint64_t off=(uint64_t)seg->offset+512u*index;
    const uint64_t addr=(uint64_t)seg->va+512u*index;
    if(off/512u+1u>UINT32_MAX||addr>UINT32_MAX)
        return PORT_DQ_AUX_FORMAT;
    uint32_t physical=0u;
    if(port_dq_dir_map_vbn(fh2,(uint32_t)(off/512u+1u),&physical)
       !=PORT_DQ_DIR_OK)return PORT_DQ_AUX_MAP;
    *lbn=physical;
    *va=(uint32_t)addr;
    *valid=seg->filesz-512u*index>=512u?512u:seg->filesz-512u*index;
    return PORT_DQ_AUX_OK;
}
