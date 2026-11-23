#include "vms_dq_imageprobe.h"
uint32_t port_dq_image_first_data(const uint8_t header[512],uint32_t *lbn){
    if(!header||!lbn)return PORT_DQ_IMAGE_BAD;
    if(port_dq_dir_map_vbn(header,1u,lbn)!=PORT_DQ_DIR_OK)
        return PORT_DQ_IMAGE_MAP;
    return PORT_DQ_IMAGE_OK;
}
