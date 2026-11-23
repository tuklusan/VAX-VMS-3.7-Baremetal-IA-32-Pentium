#include "vms_rtf_readlbn.h"
#include <stddef.h>
uint32_t port_rtf_target_dev(struct port_rtf_target *target,
                             const uint8_t *name,uint16_t length){
    if(!target || (length && !name) || length>sizeof target->name)
        return PORT_SS_BADPARAM;
    target->name_size=length;
    for(unsigned i=0;i<sizeof target->name;i++)target->name[i]=0;
    for(unsigned i=0;i<length;i++)target->name[i]=name[i];
    return PORT_SS_NORMAL;
}
uint32_t port_rtf_read_lbn(const struct port_rtf_target *target,uint32_t lbn,
                           uint8_t data[512],const struct port_rtf_read_ops *ops){
    if(!target || !target->name_size || !data || !ops || !ops->assign ||
       !ops->read_lbn || !ops->deassign)return PORT_SS_BADPARAM;
    uint16_t chan=0,io_status=0;
    uint32_t status=ops->assign(ops->context,target->name,target->name_size,&chan);
    if(!(status&1u))return status;
    status=ops->read_lbn(ops->context,chan,lbn,data,&io_status);
    if(!(status&1u))return status;
    if(!(io_status&1u))return io_status;
    status=ops->deassign(ops->context,chan);
    return status;
}
