#include "vms_ioc_phys.h"
/* IOSUBPAGD.MAR 508-531 SEARCHUNIT: traverse DDB$L_UCB then UCB$L_LINK,
 * checking typed UCBs and unit numbers. Generic use needs an available unit;
 * source DEVICEDAT mailbox/console templates already have REFC=1. */
uint32_t port_v37_ioc_ucb(const uint8_t *m,size_t n,uint32_t base,
        const struct port_v37_device_result *r,unsigned ddb_idx,
        uint16_t unit,unsigned mode,uint32_t *address){
    if(!address||ddb_idx>=PORT_V37_DEVICE_DDB_COUNT||
       (mode!=PORT_V37_IOC_MODE_PHYSICAL&&mode!=PORT_V37_IOC_MODE_GENERIC&&mode!=PORT_V37_IOC_MODE_ALLOCATION)||
       port_v37_ioc_validate(m,n,base,r)!=PORT_V37_IOC_OK)
        return PORT_V37_IOC_BAD;
    uint32_t link=port_v37_read32(m+(r->ddb[ddb_idx]-base)+4u);
    unsigned walked=0u,seen_ref=0u;
    while(link&&walked<PORT_V37_DEVICE_UCB_COUNT){
        if(link<base||link-base>PORT_V37_DEVICE_BYTES-PORT_V37_UCB_SIZE)
            return PORT_V37_IOC_BAD;
        uint32_t off=link-base;
        const uint8_t *u=m+off;
        uint16_t declared=port_v37_read16(u+8u);
        if(declared<PORT_V37_UCB_SIZE||declared>n-off||u[10u]!=PORT_V37_DYN_UCB||
           port_v37_read32(u+36u)!=r->ddb[ddb_idx])return PORT_V37_IOC_BAD;
        uint16_t ref=port_v37_read16(u+80u);
        if(mode==PORT_V37_IOC_MODE_PHYSICAL){
            if(port_v37_read16(u+72u)==unit){*address=link;return PORT_V37_IOC_OK;}
        }else if(ref==0u){
            *address=link;return PORT_V37_IOC_OK;
        }else seen_ref=1u;
        uint32_t next=port_v37_read32(u+44u);
        if(next==link)return PORT_V37_IOC_BAD;
        link=next;
        walked++;
    }
    if(link)return PORT_V37_IOC_BAD;
    return seen_ref?PORT_V37_IOC_UNAVAILABLE:PORT_V37_IOC_NO_SUCH;
}
