#include "vms_iogen_readdriv.h"
#include <stddef.h>
uint32_t port_iogen_readdriv(const uint8_t *name,uint8_t acf_flags,
                             uint16_t *channel,uint32_t range[2],
                             const struct port_iogen_readdriv_ops *ops){
    if(!name || !channel || !range || !ops || !ops->open || !ops->map_section)
        return PORT_SS_BADPARAM;
    unsigned length=name[0];
    if(!length)return PORT_SS_BADPARAM;
    uint16_t ch=0;
    uint32_t status=ops->open(ops->context,(const char *)name+1,
                              (uint16_t)length,&ch);
    if(!(status&1u)){
        if(!(acf_flags&(PORT_IOGEN_NOLOAD_DB|PORT_IOGEN_NOSUPPORT)) &&
           ops->signal_open_error)ops->signal_open_error(ops->context,status);
        return status;
    }
    *channel=ch;
    /* CRMPSC expands the P0 region from virtual block one. */
    range[0]=range[1]=0u;
    return ops->map_section(ops->context,ch,range);
}
