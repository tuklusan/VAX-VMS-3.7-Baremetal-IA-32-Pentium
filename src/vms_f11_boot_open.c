#include "vms_f11_bootpath.h"
/* Read-only chain: validated MFD FID 4,4,0 -> directory FID and FH2 ->
 * file FID and FH2. Each physical sector comes from the caller's live
 * native ATA device; unlike cached bootstrap probes no disk bytes are
 * borrowed from a prior caller. */
uint32_t port_f11_boot_open(const struct port_f11_boot_volume *v,
    const char *directory,size_t dir_len,const char *file,size_t file_len,
    uint16_t version,struct port_f11_boot_file *out){
    if(!v||!v->home||!v->index||!v->mfd||!v->read_sector||!out||
       !v->max_directory_vbn||v->max_directory_vbn>65535u||!version||
       port_f11_boot_validate_name(directory,dir_len)!=PORT_F11_PATH_OK||
       port_f11_boot_validate_name(file,file_len)!=PORT_F11_PATH_OK)
        return PORT_F11_PATH_BAD;
    const struct port_dq_dir_fid root={4u,4u,0u};
    if(port_dq_dir_validate_header(v->mfd,&root)!=PORT_DQ_DIR_OK)
        return PORT_F11_PATH_HEADER;
    struct port_f11_boot_file temp={0};
    uint8_t dir_header[512];
    uint32_t s=port_f11_boot_lookup(v,v->mfd,directory,dir_len,1u,
                                   &temp.directory,&temp.directory_vbn);
    if(s!=PORT_F11_PATH_OK)return s;
    s=port_f11_boot_header(v,&temp.directory,&temp.directory_header_lbn,
                           dir_header);
    if(s!=PORT_F11_PATH_OK)return s;
    if(port_dq_dir_validate_header(dir_header,&temp.directory)!=PORT_DQ_DIR_OK)
        return PORT_F11_PATH_HEADER;
    s=port_f11_boot_lookup(v,dir_header,file,file_len,version,
                           &temp.file,&temp.file_vbn);
    if(s!=PORT_F11_PATH_OK)return s;
    s=port_f11_boot_header(v,&temp.file,&temp.file_header_lbn,temp.header);
    if(s!=PORT_F11_PATH_OK)return s;
    *out=temp;
    return PORT_F11_PATH_OK;
}
