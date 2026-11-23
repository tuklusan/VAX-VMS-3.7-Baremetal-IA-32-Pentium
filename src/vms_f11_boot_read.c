#include "vms_f11_bootpath.h"
/* FIL$READVBN: preserve exact short-I/O count; only verified open FH2 can
 * supply the retrieval map. No process or channel record is allocated. */
uint32_t port_f11_boot_read(const struct port_f11_boot_volume *v,
    const struct port_f11_boot_file *f,uint32_t off,uint32_t count,
    uint8_t *out,size_t capacity,uint32_t *completed){
    if(!v||!f||!v->read_sector||!f->file.number||!f->file.sequence||
       !out||!completed||count>capacity||
       port_dq_image_verify_fh2(f->header,&f->file)!=PORT_DQ_IMAGE_OK)
        return PORT_F11_PATH_BAD;
    uint32_t n=0u;
    const uint32_t rc=port_f11_read_range(f->header,off,count,out,capacity,
          v->read_sector,v->context,&n);
    if(rc==PORT_F11_RANGE_BAD)return PORT_F11_PATH_BAD;
    if(rc==PORT_F11_RANGE_MAP)return PORT_F11_PATH_MAP;
    *completed=n;
    if(rc==PORT_F11_RANGE_IO)return PORT_F11_PATH_IO;
    return PORT_F11_PATH_OK;
}
