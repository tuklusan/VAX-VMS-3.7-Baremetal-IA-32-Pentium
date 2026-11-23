#include "vms_exe_dclcmh.h"
#include <stddef.h>
uint32_t port_exe_dclcmh(struct port_exe_change_handlers *h,
                         uint32_t new_address,uint32_t old_address,
                         uint32_t type,const struct port_exe_access *user,
                         const struct port_exe_bad_service *bad) {
    if(!h)return PORT_SS_BADPARAM;
    uint32_t *slot=&h->compatibility;
    if(!(type&1u)) {
        if((h->previous_mode&3u)<2u) {
            if(bad && bad->bugcheck)bad->bugcheck(bad->context);
            return PORT_SS_IVSSRQ;
        }
        slot=(h->previous_mode&3u)==2u?&h->supervisor:&h->user;
    }
    if(old_address &&
       (!port_exe_probe_write(user,old_address,4u) ||
        !port_exe_write(user,old_address,slot,4u)))
        return PORT_SS_ACCVIO;
    *slot=new_address;
    return PORT_SS_NORMAL;
}
