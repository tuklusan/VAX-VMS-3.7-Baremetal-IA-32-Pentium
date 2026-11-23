#include "vms_ioc_phys.h"
/* IO database search is read-only; validate the full resident source DDB/
 * UCB layout before following any 32-bit virtual link from a guest image. */
uint32_t port_v37_ioc_validate(const uint8_t *m,size_t n,uint32_t base,
                               const struct port_v37_device_result *r){
    if(!m||!r||n<PORT_V37_DEVICE_BYTES||!base||base&3u||
       r->bytes!=PORT_V37_DEVICE_BYTES||r->list_head!=base||
       port_v37_devices_check(m,n,base,r)!=PORT_V37_DEV_OK)
        return PORT_V37_IOC_BAD;
    return PORT_V37_IOC_OK;
}
