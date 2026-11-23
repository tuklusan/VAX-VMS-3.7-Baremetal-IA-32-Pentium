#include "vms_dq_imageprobe.h"
/* Source file-header VBN = IBMAPSIZE + 4*CLUSTER + FID$W_NUM. */
uint32_t port_dq_image_header_lbn(const uint8_t home[512],
    const uint8_t index[512],const struct port_dq_dir_fid *fid,
    uint32_t *lbn){
    if(!home||!index||!fid||!lbn||!fid->number||!fid->sequence)
        return PORT_DQ_IMAGE_BAD;
    uint32_t index_physical=0u;
    if(port_dq_index_location(home,&index_physical)!=PORT_DQ_INDEX_OK)
        return PORT_DQ_IMAGE_HEADER;
    const uint64_t vbn=(uint64_t)port_dq_index_word(home,32u)+
        4u*(uint64_t)port_dq_index_word(home,14u)+fid->number;
    if(vbn==0u||vbn>UINT32_MAX ||
       port_dq_dir_map_vbn(index,(uint32_t)vbn,lbn)!=PORT_DQ_DIR_OK)
        return PORT_DQ_IMAGE_MAP;
    return PORT_DQ_IMAGE_OK;
}
uint32_t port_dq_image_verify_fh2(const uint8_t header[512],
                                 const struct port_dq_dir_fid *fid){
    if(!header||!fid||!fid->number||!fid->sequence)return PORT_DQ_IMAGE_BAD;
    if(header[7u]!=2u||header[0u]==0u||header[1u]<=header[0u]||
       header[1u]>=255u||
       port_dq_index_word(header,8u)!=fid->number||
       port_dq_index_word(header,10u)!=fid->sequence||
       port_dq_index_word(header,12u)!=fid->volume||
       !port_dq_index_checksum(header,510u))return PORT_DQ_IMAGE_HEADER;
    return PORT_DQ_IMAGE_OK;
}
