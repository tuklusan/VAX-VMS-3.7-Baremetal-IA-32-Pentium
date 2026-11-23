#include "vms_f11_bootpath.h"
/* FIL$FINDFILID scans every requested directory VBN through the source FH2
 * map and the actual physical-sector callback. */
uint32_t port_f11_boot_lookup(const struct port_f11_boot_volume *v,
    const uint8_t dir[512],const char *name,size_t n,uint16_t version,
    struct port_dq_dir_fid *fid,uint32_t *found_vbn){
    if(!v||!dir||!v->read_sector||!v->max_directory_vbn||
       !version||!fid||!found_vbn||
       port_f11_boot_validate_name(name,n)!=PORT_F11_PATH_OK)
        return PORT_F11_PATH_BAD;
    struct port_dq_dir_fid tmp={0};uint32_t vbn=0;
    const uint32_t result=port_dq_dir_find_blocks(dir,name,n,version,
       v->max_directory_vbn,v->read_sector,v->context,&tmp,&vbn);
    if(result==PORT_DQ_DIR_NOT_FOUND)return PORT_F11_PATH_MISSING;
    if(result!=PORT_DQ_DIR_OK)return PORT_F11_PATH_MAP;
    *fid=tmp;*found_vbn=vbn;
    return PORT_F11_PATH_OK;
}
