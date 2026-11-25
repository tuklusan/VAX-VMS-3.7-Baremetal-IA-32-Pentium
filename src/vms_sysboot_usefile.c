#include "vms_sysboot_paramfile.h"
#include "vms_sysboot_read.h"

uint32_t port_sysboot_usefile(const struct boo_io_driver *disk,uint32_t fn,
    const struct port_f11_volume *volume,const char *filespec,
    struct port_vms37_init_params *params,uint8_t startup_file[32],
    uint32_t *applied_count,uint8_t *buffer,size_t capacity,
    size_t *file_bytes){
    if(!disk||!volume||!filespec||!params||!startup_file||
       !applied_count||!buffer||!file_bytes)
        return PORT_SYSBOOT_USEFILE_BAD;
    struct port_f11_fid fid={0};
    uint8_t header[512];uint32_t first=0u,allocated=0u;
    uint32_t status=port_f11_open_named_file(disk,fn,volume,filespec,
        &fid,header,&first,&allocated);
    if(status==PORT_F11_FILE_NOT_FOUND)
        return PORT_SYSBOOT_USEFILE_NOT_FOUND;
    if(status!=PORT_F11_HOME_OK)return PORT_SYSBOOT_USEFILE_IO;
    (void)first;(void)allocated;
    struct port_f11_extent extents[PORT_SYSBOOT_MAX_EXTENTS];
    uint32_t count=0u,blocks=0u,start=0u;
    status=port_f11_collect_file_extents(disk,fn,volume,fid,extents,
        PORT_SYSBOOT_MAX_EXTENTS,&count,&blocks,&start);
    if(status!=PORT_F11_HOME_OK||!count||!blocks)
        return PORT_SYSBOOT_USEFILE_IO;
    (void)start;
    const uint64_t required=(uint64_t)blocks*512u;
    if(required>SIZE_MAX||required>capacity)
        return PORT_SYSBOOT_USEFILE_SMALL;
    size_t output_block=0u;
    for(uint32_t i=0u;i<count;i++){
        if(output_block>blocks||
           (size_t)extents[i].blocks>(size_t)blocks-output_block)
            return PORT_SYSBOOT_USEFILE_IO;
        for(uint32_t b=0u;b<extents[i].blocks;b++){
            const uint32_t lbn=extents[i].lbn+b;
            if(!(boo_rdwr_lbn(disk,lbn,buffer+output_block*512u,fn,512u)&1u))
                return PORT_SYSBOOT_USEFILE_IO;
            output_block++;
        }
    }
    if(output_block!=blocks)return PORT_SYSBOOT_USEFILE_IO;
    uint32_t applied=0u;
    status=port_vms37_init_params_apply_file(params,buffer,
        (size_t)blocks*512u,startup_file,&applied);
    if(status!=PORT_VMS37_SYSPARAM_FILE_OK)
        return PORT_SYSBOOT_USEFILE_PARAMETERS;
    *applied_count=applied;
    *file_bytes=(size_t)required;
    return PORT_SYSBOOT_USEFILE_OK;
}
