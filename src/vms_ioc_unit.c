#include "vms_ioc_phys.h"
/* IOSUBPAGD.MAR 428-443: two-stage unsigned decimal unit conversion
 * with carry/overflow refusal rather than wrapping the 16-bit unit. */
uint32_t port_v37_ioc_parse_unit(const char *s,size_t n,uint16_t *unit,uint8_t *explicit_unit){
    if(!s||!unit||!explicit_unit)return PORT_V37_IOC_BAD;
    uint32_t v=0u;
    for(size_t i=0;i<n;i++){
        uint8_t ch=(uint8_t)s[i];
        if(ch<'0'||ch>'9')return PORT_V37_IOC_BAD;
        uint32_t digit=ch-'0';
        if(v>(65535u-digit)/10u)return PORT_V37_IOC_BAD;
        v=v*10u+digit;
    }
    *unit=(uint16_t)v;
    *explicit_unit=(uint8_t)(n!=0u);
    return PORT_V37_IOC_OK;
}
