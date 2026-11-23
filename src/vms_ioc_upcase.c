#include "vms_ioc_phys.h"
/* IOSUBPAGD.MAR 329-348: copy and uppercase ASCII, one bounded pass. */
uint32_t port_v37_ioc_upcase(const char *s,size_t n,char *out,size_t cap){
    if(!s||!out||!n||n>PORT_V37_IOC_NAME_MAX||cap<n+1u)
        return PORT_V37_IOC_BAD;
    for(size_t i=0;i<n;i++){
        unsigned char ch=(unsigned char)s[i];
        if(!ch)return PORT_V37_IOC_BAD;
    }
    for(size_t i=0;i<n;i++){
        uint8_t ch=(uint8_t)s[i];
        out[i]=(char)(ch>='a'&&ch<='z' ? (uint8_t)(ch-32u):ch);
    }
    out[n]='\0';
    return PORT_V37_IOC_OK;
}
