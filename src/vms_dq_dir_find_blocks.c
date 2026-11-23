#include "vms_dq_dir.h"
/* FIL$FINDFILID linearly searches directory VBN records. Each VBN is
 * translated through its own source FH2 retrieval-pointer map so a
 * fragmented directory cannot accidentally alias an adjacent disk LBN.
 * The only successful mutation is the returned FID and matching VBN. */
uint32_t port_dq_dir_find_blocks(const uint8_t fh2[512],
    const char *name,size_t length,uint16_t version,uint32_t max_vbn,
    port_dq_dir_sector_read_fn read_sector,void *context,
    struct port_dq_dir_fid *out,uint32_t *found_vbn){
    if(!fh2||!name||!length||length>80u||!version||!max_vbn||
       max_vbn>UINT32_C(65535)||!read_sector||!out||!found_vbn)
        return PORT_DQ_DIR_BAD;
    for(uint32_t vbn=1u;vbn<=max_vbn;vbn++){
        uint32_t physical=0u;
        if(port_dq_dir_map_vbn(fh2,vbn,&physical)!=PORT_DQ_DIR_OK)
            return PORT_DQ_DIR_MAP;
        uint8_t data[512];
        if(read_sector(context,physical,data)!=PORT_DQ_DIR_OK)
            return PORT_DQ_DIR_MAP;
        struct port_dq_dir_fid local={0};
        const uint32_t result=port_dq_dir_find(data,name,length,version,&local);
        if(result==PORT_DQ_DIR_OK){
            *out=local;*found_vbn=vbn;
            return PORT_DQ_DIR_OK;
        }
        if(result!=PORT_DQ_DIR_NOT_FOUND)return result;
    }
    return PORT_DQ_DIR_NOT_FOUND;
}
