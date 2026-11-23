#include "vms_sysqio_args.h"
#include "vms_sys85.h"

/* SYSQIOREQ.MAR 515-581. Volume ACL details remain in the executive's
 * CHKPHYACCES/CHKLOGACCES callback; this routine preserves when each check
 * is selected and the source DEV characteristic gates around it. */
uint32_t port_v37_sysqio_device_access(uint16_t function,
    uint32_t characteristics,uint8_t has_phy_io,uint8_t has_log_io,
    uint16_t protection,uint32_t owner_uic,uint32_t arb_va,
    port_v37_sysqio_volume_check_fn check_volume,void *context,
    uint8_t *physical_io,uint32_t *service_status){
    if(!physical_io||!service_status||has_phy_io>1u||has_log_io>1u)
        return PORT_V37_SYSQIO_DEVICE_BAD;
    const uint8_t code=(uint8_t)(function&63u);
    const uint8_t is_physical=(uint8_t)(code<=31u);
    if(has_phy_io||code>47u){
        *physical_io=is_physical;
        *service_status=PORT_SYS85_NORMAL;
        return PORT_V37_SYSQIO_DEVICE_OK;
    }
    uint8_t access_kind;
    if(is_physical){
        if(!has_log_io){
            *service_status=PORT_SYS85_NOPRIV;
            return PORT_V37_SYSQIO_DEVICE_NOPRIV;
        }
        access_kind=PORT_V37_SYSQIO_ACCESS_PHYSICAL;
    }else{
        if(has_log_io){
            *physical_io=0u;
            *service_status=PORT_SYS85_NORMAL;
            return PORT_V37_SYSQIO_DEVICE_OK;
        }
        access_kind=PORT_V37_SYSQIO_ACCESS_LOGICAL;
    }
    if(characteristics&PORT_V37_DEVCHAR_SPL){
        *service_status=PORT_SYS85_NOPRIV;
        return PORT_V37_SYSQIO_DEVICE_NOPRIV;
    }
    if(!(characteristics&PORT_V37_DEVCHAR_FOD)){
        if(!(characteristics&PORT_V37_DEVCHAR_SHR)){
            *physical_io=is_physical;
            *service_status=PORT_SYS85_NORMAL;
            return PORT_V37_SYSQIO_DEVICE_OK;
        }
    }else if(!(characteristics&PORT_V37_DEVCHAR_MNT)||
             !(characteristics&PORT_V37_DEVCHAR_FOR)){
        *service_status=PORT_SYS85_NOPRIV;
        return PORT_V37_SYSQIO_DEVICE_NOPRIV;
    }
    if(!check_volume)return PORT_V37_SYSQIO_DEVICE_BAD;
    const uint32_t checked=check_volume(context,access_kind,protection,
                                         owner_uic,arb_va);
    if(!(checked&1u)){
        *service_status=PORT_SYS85_NOPRIV;
        return PORT_V37_SYSQIO_DEVICE_NOPRIV;
    }
    *physical_io=is_physical;
    *service_status=PORT_SYS85_NORMAL;
    return PORT_V37_SYSQIO_DEVICE_OK;
}
