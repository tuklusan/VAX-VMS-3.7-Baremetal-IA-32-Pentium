#include "vms_sysboot_read.h"
#include "vms_sysboot_map.h"
/* SYSBOOT READ_VIRTUAL uses the normalized map returned by FILEREAD,
 * including extension file headers. The driver's transfer is a real QIO. */
static uint32_t sysboot_read_map(const struct boo_io_driver *disk,
                                uint32_t fn,const struct port_f11_extent *map,
                                uint32_t count,uint32_t vbn,uint32_t bytes,
                                uint8_t *out,uint32_t capacity){
    if(!disk||!map||!count||!vbn||(!out&&bytes))
        return PORT_SYSBOOT_BAD_INPUT;
    if(bytes>capacity)return PORT_SYSBOOT_SMALL_BUFFER;
    if(!bytes)return PORT_F11_HOME_OK;
    uint32_t blocks=bytes/512u+(bytes%512u!=0u);
    if(blocks>UINT32_MAX-vbn+1u)return PORT_SYSBOOT_BAD_PARAMETERS;
    uint32_t remaining=0,index=0,lbn=0;
    uint32_t status=port_sysboot_mapvbn(map,count,vbn,&lbn,&remaining,&index);
    if(status!=PORT_F11_HOME_OK)return status;
    uint8_t tail[512];
    for(uint32_t i=0;i<blocks;i++){
        if(!remaining){
            if(index>=count)return PORT_F11_FILE_EOF;
            lbn=map[index].lbn;remaining=map[index].blocks;
            index++;
        }
        uint32_t leftover=bytes-i*512u;
        void *dest=leftover>=512u?out+i*512u:tail;
        status=boo_rdwr_lbn(disk,lbn,dest,fn,512u);
        if(!(status&1u))return PORT_F11_MOUNT_DISK_ERROR;
        if(leftover<512u)
            for(uint32_t j=0;j<leftover;j++)out[i*512u+j]=tail[j];
        lbn++;remaining--;
    }
    return PORT_F11_HOME_OK;
}
/* SYSBOOT.MAR IMAGE_OPEN, IMAGE_INIT: read the actual file header and
 * image-header VBN, and determine the section of disk bytes to load. */
uint32_t port_sysboot_image_init(const struct boo_io_driver *disk,
                                 uint32_t read_function,
                                 const struct port_f11_volume *volume,
                                 const char *filespec,
                                 const struct boo_image_layout *layout,
                                 uint8_t file_header[512],
                                 struct port_sysboot_read_result *found) {
    if(!disk||!volume||!filespec||!layout||!file_header||!found)
        return PORT_SYSBOOT_BAD_INPUT;
    uint32_t first=0,blocks=0;
    uint32_t rc=port_f11_open_named_file(disk,read_function,volume,filespec,
                &found->sys_file,file_header,&first,&blocks);
    if(rc!=PORT_F11_HOME_OK)return rc;
    rc=port_f11_collect_file_extents(disk,read_function,volume,
                        found->sys_file,found->extents,PORT_SYSBOOT_MAX_EXTENTS,
                        &found->extent_count,&blocks,&first);
    if(rc!=PORT_F11_HOME_OK)return rc;
    uint8_t image_header[512];
    rc=sysboot_read_map(disk,read_function,found->extents,
                  found->extent_count,1u,512u,image_header,512u);
    if(rc!=PORT_F11_HOME_OK)return rc;
    uint32_t last=blocks,header_pages=0;
    if(!boo_image_attributes(image_header,512u,layout,&last,&header_pages) ||
       !header_pages || header_pages>=last)
        return PORT_SYSBOOT_BAD_IMAGE;
    found->last_image_vbn=last;
    found->image_vbn=header_pages+1u;
    found->sysparam_vbn=0;
    return PORT_F11_HOME_OK;
}
/* READ_VIRTUAL and BOO$MAPVBN: each VBN maps through the original
 * Files-11 retrieval pointers. Reads never cross the caller's buffer. */
uint32_t port_sysboot_read_virtual(const struct boo_io_driver *disk,
                                   uint32_t read_function,
                                   const uint8_t file_header[512],
                                   uint32_t start_vbn,uint32_t bytes,
                                   uint8_t *output,uint32_t capacity) {
    if(!disk||!file_header||(!output&&bytes)||!start_vbn)
        return PORT_SYSBOOT_BAD_INPUT;
    if(bytes>capacity)return PORT_SYSBOOT_SMALL_BUFFER;
    if(!bytes)return PORT_F11_HOME_OK;
    struct port_f11_extent extents[PORT_SYSBOOT_MAX_EXTENTS];
    uint32_t used=0;
    uint32_t rc=port_f11_collect_extents(file_header,extents,
                                          PORT_SYSBOOT_MAX_EXTENTS,&used);
    if(rc!=PORT_F11_HOME_OK)return rc;
    return sysboot_read_map(disk,read_function,extents,used,
                              start_vbn,bytes,output,capacity);
}
/* LOAD_CODE: load the whole executable region, excluding image headers,
 * debug records and patch records. Calling INIT is a separate step. */
uint32_t port_sysboot_load_code(const struct boo_io_driver *disk,
                                uint32_t read_function,
                                const uint8_t file_header[512],
                                const struct port_sysboot_read_result *image,
                                uint8_t *output,uint32_t capacity,
                                uint32_t *loaded_bytes) {
    if(!image||!loaded_bytes||!output||!file_header||!disk ||
       !image->image_vbn||image->last_image_vbn<image->image_vbn)
        return PORT_SYSBOOT_BAD_INPUT;
    uint32_t blocks=image->last_image_vbn-image->image_vbn+1u;
    if(blocks>UINT32_MAX/512u)return PORT_SYSBOOT_BAD_IMAGE;
    uint32_t bytes=blocks*512u;
    if(bytes>capacity)return PORT_SYSBOOT_SMALL_BUFFER;
    uint32_t rc=sysboot_read_map(disk,read_function,image->extents,
                 image->extent_count,image->image_vbn,bytes,output,capacity);
    if(rc==PORT_F11_HOME_OK)*loaded_bytes=bytes;
    return rc;
}
uint32_t port_sysboot_getcurrent(const struct boo_io_driver *disk,
                                 uint32_t read_function,
                                 const struct port_f11_volume *volume,
                                 const char *filespec,
                                 const struct port_sysboot_param_location *loc,
                                 uint8_t *parameters,
                                 uint32_t available,
                                 struct port_sysboot_read_result *found) {
    if(!disk || !volume || !filespec || !loc || !parameters || !found ||
       !loc->sysparam_byte_count || loc->sysparam_byte_count>available ||
       loc->sysparam_image_offset & 511u)
        return PORT_SYSBOOT_BAD_INPUT;
    uint8_t file_header[512];
    uint32_t rc=port_sysboot_image_init(disk,read_function,volume,filespec,
                                         &loc->header,file_header,found);
    if(rc!=PORT_F11_HOME_OK)return rc;
    /* SYSBOOT GETCURRENT builds the SYSPARAM file VBN by adding
     * MMG$A_SYSPARAM's image-relative page offset to the image start. */
    uint32_t delta=loc->sysparam_image_offset/512u;
    if(delta>UINT32_MAX-found->image_vbn)return PORT_SYSBOOT_BAD_PARAMETERS;
    found->sysparam_vbn=found->image_vbn+delta;
    uint32_t blocks=(loc->sysparam_byte_count+511u)/512u;
    if(!blocks || found->sysparam_vbn>found->last_image_vbn ||
       blocks>found->last_image_vbn-found->sysparam_vbn+1u)
        return PORT_SYSBOOT_BAD_PARAMETERS;
    return sysboot_read_map(disk,read_function,found->extents,
                        found->extent_count,found->sysparam_vbn,
                        loc->sysparam_byte_count,parameters,available);
}
