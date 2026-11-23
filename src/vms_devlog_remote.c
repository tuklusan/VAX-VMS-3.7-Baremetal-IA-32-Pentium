#include "vms_devlog_search.h"
/* IOSUBPAGD.MAR 150$-170$,250$: double colon is a remote device. */
uint32_t port_devlog_remote(const char *name,size_t n){
    if(!name||!n||n>PORT_V37_IOC_NAME_MAX)return PORT_V37_IOC_BAD;
    for(size_t i=1;i<n;i++)if(name[i-1u]==':'&&name[i]==':')
        return PORT_V37_IOC_NONLOCAL;
    return PORT_V37_IOC_OK;
}
