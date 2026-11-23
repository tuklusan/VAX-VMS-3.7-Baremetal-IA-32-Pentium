#include "vms_log_system.h"
/* LOG$TRANSLATE lines 404-411: length is returned even if buffer too
 * small. Do NOT partially write, do not silently truncate a logical name. */
uint32_t port_log_copy_result(const uint8_t *from,size_t needed,
    uint8_t *to,size_t capacity,uint32_t status,uint32_t source_address,
    uint8_t access_mode,struct port_log_result *out){
    if(!from||!to||!out||needed>PORT_LOG_MAX_NAME||
       (status!=PORT_LOG_OK&&status!=PORT_LOG_NOTRAN))return PORT_LOG_BAD;
    struct port_log_result result={.status=status,
        .record_address=source_address,.bytes=(uint16_t)needed,
        .table=(uint8_t)(status==PORT_LOG_OK?PORT_LOG_SYSTEM:0u),
        .access_mode=access_mode};
    if(needed>capacity){result.status=PORT_LOG_OVERFLOW;*out=result;
        return PORT_LOG_OVERFLOW;}
    for(size_t i=0u;i<needed;i++)to[i]=from[i];
    *out=result;
    return status;
}
