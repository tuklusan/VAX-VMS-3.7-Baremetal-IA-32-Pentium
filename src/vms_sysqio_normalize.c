#include "vms_sysqio_args.h"
#include "vms_dq_fdt.h"
#include "vms_sys85.h"

/* SYSQIOREQ.MAR ACCESS (450-479): a no-window virtual write during early
 * system initialization must become a logical write only for a mounted
 * foreign FOD or a device that is not file oriented. */
uint32_t port_v37_sysqio_normalize_write(uint16_t function,
    uint32_t device_characteristics,uint32_t window_address,
    uint16_t *normalized_function,uint8_t *ast_quota_check,
    uint32_t *service_status){
    if(!normalized_function||!ast_quota_check||!service_status)
        return PORT_V37_SYSQIO_ACCESS_BAD;
    uint16_t normalized=function;
    uint8_t check_ast=0u;
    uint32_t status=PORT_SYS85_NORMAL;
    if((function&63u)==PORT_DQ_WRITEVBLK){
        if(device_characteristics&PORT_V37_DEVCHAR_FOD){
            if(window_address){
                check_ast=1u;
                goto success;
            }
            if(!(device_characteristics&PORT_V37_DEVCHAR_MNT)){
                status=PORT_SYS85_NOPRIV;
                *service_status=status;
                return PORT_V37_SYSQIO_ACCESS_NOPRIV;
            }
            if(!(device_characteristics&PORT_V37_DEVCHAR_FOR))
                goto success;
        }
        normalized=(uint16_t)(function-16u);
        if(!(device_characteristics&(PORT_V37_DEVCHAR_SPL|
             PORT_V37_DEVCHAR_FOD|PORT_V37_DEVCHAR_SHR)))
            check_ast=1u;
    }
success:
    *normalized_function=normalized;
    *ast_quota_check=check_ast;
    *service_status=status;
    return PORT_V37_SYSQIO_ACCESS_OK;
}
