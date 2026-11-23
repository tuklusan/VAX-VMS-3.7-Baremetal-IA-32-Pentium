#include "vms_ioc_phys.h"
/* IOSUBPAGD.MAR 362-384: one underscore forces physical name; two
 * additionally mark a concealed device. A third is invalid, never skipped. */
uint32_t port_v37_ioc_strip(const char *s,size_t n,unsigned *prefix,size_t *start){
    if(!s||!n||!prefix||!start)return PORT_V37_IOC_BAD;
    unsigned k=0u;
    while(k<n&&s[k]=='_'&&k<3u)k++;
    if(!k&&s[0]=='\0')return PORT_V37_IOC_BAD;
    if(k>2u||k==n)return PORT_V37_IOC_BAD;
    *prefix=k;
    *start=k;
    return PORT_V37_IOC_OK;
}
