#include "vms_ioc_phys.h"
/* IOSUBPAGD.MAR 397-460: parse the *physical* part only; network ::
 * and names requiring LOG$TRANSLATE are not silently made local. */
uint32_t port_v37_ioc_parse(const char *name,size_t n,struct port_v37_ioc_name *out){
    if(!out||!name||n==0u||n>PORT_V37_IOC_NAME_MAX)return PORT_V37_IOC_BAD;
    char buf[PORT_V37_IOC_NAME_MAX+1u];
    if(port_v37_ioc_upcase(name,n,buf,sizeof buf)!=PORT_V37_IOC_OK)
        return PORT_V37_IOC_BAD;
    /* In the original physical lookup, a nodename followed by :: belongs
     * to the network stack and must not be resolved through local DDBs. */
    for(size_t i=1;i<n;i++)if(buf[i-1u]==':'&&buf[i]==':')
        return PORT_V37_IOC_NONLOCAL;
    unsigned prefix=0u;size_t start=0u;
    if(port_v37_ioc_strip(buf,n,&prefix,&start)!=PORT_V37_IOC_OK)
        return PORT_V37_IOC_BAD;
    size_t end=n;
    if(buf[end-1u]==':')end--;
    if(end<=start)return PORT_V37_IOC_BAD;
    for(size_t i=start;i<end;i++){
        if(buf[i]=='$')return PORT_V37_IOC_LOGICAL_PENDING;
        if(buf[i]==':')return PORT_V37_IOC_BAD;
    }
    size_t split=start;
    while(split<end&&buf[split]>='A'&&buf[split]<='Z')split++;
    size_t nalpha=split-start;
    if(nalpha==0u||nalpha>15u)return PORT_V37_IOC_BAD;
    for(size_t i=split;i<end;i++)if(buf[i]<'0'||buf[i]>'9')
        return PORT_V37_IOC_BAD;
    struct port_v37_ioc_name tmp={0};
    for(size_t i=0;i<nalpha;i++)tmp.controller[i]=buf[start+i];
    tmp.length=(uint8_t)nalpha;
    tmp.concealed=(uint8_t)(prefix==2u);
    uint32_t st=port_v37_ioc_parse_unit(buf+split,end-split,&tmp.unit,&tmp.explicit_unit);
    if(st!=PORT_V37_IOC_OK)return st;
    *out=tmp;
    return PORT_V37_IOC_OK;
}
