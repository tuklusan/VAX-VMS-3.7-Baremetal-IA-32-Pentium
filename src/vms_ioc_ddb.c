#include "vms_ioc_phys.h"
/* IOSUBPAGD.MAR 491-502 SEARCHDEV: traverse the original DDB$L_LINK
 * chain with cycle checks and counted names (NOT arbitrary C strings). */
uint32_t port_v37_ioc_ddb(const uint8_t *m,size_t n,uint32_t base,
        const struct port_v37_device_result *r,const char *name,size_t len,
        uint32_t *address,unsigned *index){
    if(!address||!index||!name||!len||len>15u||
       port_v37_ioc_validate(m,n,base,r)!=PORT_V37_IOC_OK)
        return PORT_V37_IOC_BAD;
    uint32_t link=r->list_head;
    unsigned seen=0u;
    while(link&&seen<PORT_V37_DEVICE_DDB_COUNT){
        if(link<base||link-base>PORT_V37_DEVICE_BYTES-PORT_V37_DDB_SIZE||
           (link-base)%PORT_V37_DDB_SIZE)return PORT_V37_IOC_BAD;
        unsigned idx=(link-base)/PORT_V37_DDB_SIZE;
        if(idx>=PORT_V37_DEVICE_DDB_COUNT||(seen&&(idx<seen)))
            return PORT_V37_IOC_BAD;
        const uint8_t *p=m+(link-base);
        unsigned nlen=p[20u];
        if(nlen==len){
            unsigned same=1u;
            for(unsigned i=0;i<len;i++)if((uint8_t)name[i]!=p[21u+i])same=0u;
            if(same){*address=link;*index=idx;return PORT_V37_IOC_OK;}
        }
        link=port_v37_read32(p);
        seen++;
    }
    return link?PORT_V37_IOC_BAD:PORT_V37_IOC_NO_SUCH;
}
