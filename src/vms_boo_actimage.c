#include "vms_boo_actimage.h"
#include <stddef.h>
uint32_t port_boo_actimage(uint32_t desc,
                           const struct port_boo_actimage_ops *ops){
    if(!desc || !ops || !ops->memory || !ops->enter_exec ||
       !ops->reset_exec_stack || !ops->rundown_user || !ops->activate)
        return PORT_SS_BADPARAM;
    uint8_t descriptor[8];
    if(!port_exe_read(ops->memory,desc,descriptor,8u))return PORT_SS_ACCVIO;
    uint16_t length=(uint16_t)(descriptor[0]|((uint16_t)descriptor[1]<<8));
    uint32_t address=(uint32_t)descriptor[4]|((uint32_t)descriptor[5]<<8)|
                    ((uint32_t)descriptor[6]<<16)|((uint32_t)descriptor[7]<<24);
    if(length>255u)return PORT_SS_BADPARAM;
    uint8_t name[255];
    if(length && !port_exe_read(ops->memory,address,name,length))
        return PORT_SS_ACCVIO;
    uint32_t status=ops->enter_exec(ops->context);
    if(!(status&1u))return status;
    status=ops->reset_exec_stack(ops->context);
    if(!(status&1u))return status;
    status=ops->rundown_user(ops->context);
    if(!(status&1u))return status;
    return ops->activate(ops->context,name,length);
}
