#include "vms_f11_bootpath.h"
/* FIL$RDCHKFILHDR: source INDEXF.SYS FID-number-to-index-VBN map and
 * checksummed FH2 identity, no caller state committed until verification. */
uint32_t port_f11_boot_header(const struct port_f11_boot_volume *v,
    const struct port_dq_dir_fid *fid,uint32_t *lbn,uint8_t out[512]){
    if(!v||!v->home||!v->index||!v->read_sector||!fid||!lbn||!out)
        return PORT_F11_PATH_BAD;
    uint32_t at=0u;
    if(port_dq_image_header_lbn(v->home,v->index,fid,&at)!=PORT_DQ_IMAGE_OK)
        return PORT_F11_PATH_MAP;
    uint8_t bytes[512];
    if(v->read_sector(v->context,at,bytes)!=PORT_DQ_DIR_OK)
        return PORT_F11_PATH_IO;
    if(port_dq_image_verify_fh2(bytes,fid)!=PORT_DQ_IMAGE_OK)
        return PORT_F11_PATH_HEADER;
    for(unsigned i=0;i<512u;i++)out[i]=bytes[i];
    *lbn=at;
    return PORT_F11_PATH_OK;
}
