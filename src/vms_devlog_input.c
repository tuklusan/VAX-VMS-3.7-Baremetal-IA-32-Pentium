#include "vms_devlog_search.h"
/* IOSUBPAGD.MAR 310-329,70$-90$: descriptor size, probe equivalent and
 * lowercase conversion into an owned result buffer. No untrusted pointers. */
uint32_t port_devlog_input(struct port_devlog_state *s,const char *src,size_t n){
    if(!s||!src||!n||n>PORT_V37_IOC_NAME_MAX)return PORT_V37_IOC_BAD;
    for(size_t i=0;i<n;i++){
        const unsigned char c=(unsigned char)src[i];
        if(c==0u||c>=128u)return PORT_V37_IOC_BAD;
    }
    struct port_devlog_state tmp={0};
    if(port_v37_ioc_upcase(src,n,tmp.name,sizeof tmp.name)!=PORT_V37_IOC_OK)
        return PORT_V37_IOC_BAD;
    tmp.name[n]=0;tmp.length=n;
    *s=tmp;
    return PORT_V37_IOC_OK;
}
