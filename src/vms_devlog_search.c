#include "vms_devlog_search.h"
/* Bounded original IOC$SEARCHDEV logical->physical device translation.
 * A failure changes neither the caller's result nor any UCB/CCB. The
 * diagnostic trace on NOSUCHDEV is intentionally returned so missing
 * DQ bind is observable without reporting a successful $ASSIGN. */
uint32_t port_devlog_search(const struct port_log_system_table *t,
    const uint8_t *m,size_t n,uint32_t base,
    const struct port_v37_device_result *dev,const char *text,size_t len,
    unsigned mode,struct port_devlog_trace *out){
    if(!t||!m||!dev||!out||!text||
       (mode!=PORT_V37_IOC_MODE_PHYSICAL&&
        mode!=PORT_V37_IOC_MODE_GENERIC&&
        mode!=PORT_V37_IOC_MODE_ALLOCATION)||
       port_log_system_check(t)!=PORT_LOG_OK||
       port_v37_ioc_validate(m,n,base,dev)!=PORT_V37_IOC_OK)
        return PORT_V37_IOC_BAD;
    struct port_devlog_state s={0};
    uint32_t status=port_devlog_input(&s,text,len);
    if(status!=PORT_V37_IOC_OK)return status;
    status=port_devlog_normalize(t,&s);
    if(status!=PORT_V37_IOC_OK)return status;
    struct port_devlog_trace trace={0};
    status=port_devlog_final(&s,&trace);
    if(status!=PORT_V37_IOC_OK)return status;
    status=port_devlog_lookup(m,n,base,dev,mode,&trace);
    if(status!=PORT_V37_IOC_OK&&status!=PORT_V37_IOC_UNBOUND&&
       status!=PORT_V37_IOC_NO_SUCH&&status!=PORT_V37_IOC_UNAVAILABLE)
        return status;
    *out=trace;
    return status;
}
