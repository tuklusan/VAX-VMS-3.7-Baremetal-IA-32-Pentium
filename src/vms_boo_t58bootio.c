#include "vms_boo_t58bootio.h"
#include <stddef.h>
uint32_t port_t58_read_lbn(const struct port_t58bootio *device,uint32_t lbn,uint8_t block[512]){
    if(!device || !device->read_block || !block)return PORT_SS_BADPARAM;
    /* The original kernel bootstrap always selects TU58 unit zero. */
    return device->read_block(device->context,0u,lbn,block);
}
