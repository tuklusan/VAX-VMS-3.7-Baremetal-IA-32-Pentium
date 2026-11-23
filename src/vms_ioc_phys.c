#include "vms_ioc_phys.h"
/* IOSUBPAGD.MAR 295-531: untranslatable names are rejected, then perform
 * original 32-bit linked list physical device and unit search. This is
 * read-only name lookup, not an IOC lock, $ASSIGN or live $QIO. */
uint32_t port_v37_ioc_search(const uint8_t *m,size_t n,uint32_t base,
         const struct port_v37_device_result *r,const char *text,size_t length,
         unsigned mode,struct port_v37_ioc_result *out){
    if(!out||(mode!=PORT_V37_IOC_MODE_PHYSICAL&&mode!=PORT_V37_IOC_MODE_GENERIC&&mode!=PORT_V37_IOC_MODE_ALLOCATION)||
       port_v37_ioc_validate(m,n,base,r)!=PORT_V37_IOC_OK)
        return PORT_V37_IOC_BAD;
    struct port_v37_ioc_name name={0};
    uint32_t st=port_v37_ioc_parse(text,length,&name);
    if(st!=PORT_V37_IOC_OK)return st;
    uint32_t ddb=0u,ucb=0u;unsigned idx=0u;
    st=port_v37_ioc_ddb(m,n,base,r,name.controller,name.length,&ddb,&idx);
    /* Original physical-name search also tries the trailing controller-A
     * expansion; apply only to source name ending in A (OPA, MBA, NLA). */
    if(st==PORT_V37_IOC_NO_SUCH&&name.length<15u&&mode==PORT_V37_IOC_MODE_PHYSICAL){
        char expanded[16];
        for(unsigned i=0;i<name.length;i++)expanded[i]=name.controller[i];
        expanded[name.length]='A';
        st=port_v37_ioc_ddb(m,n,base,r,expanded,name.length+1u,&ddb,&idx);
    }
    if(st!=PORT_V37_IOC_OK)return st;
    st=port_v37_ioc_ucb(m,n,base,r,idx,name.unit,mode,&ucb);
    if(st!=PORT_V37_IOC_OK)return st;
    const struct port_v37_ioc_result candidate={.ddb=ddb,.ucb=ucb,
        .unit=port_v37_read16(m+(ucb-base)+72u),
        .concealed=name.concealed,
        .driver_bound=(uint8_t)((r->incomplete_driver_mask&(1u<<idx))==0u)};
    *out=candidate;
    return candidate.driver_bound?PORT_V37_IOC_OK:PORT_V37_IOC_UNBOUND;
}
