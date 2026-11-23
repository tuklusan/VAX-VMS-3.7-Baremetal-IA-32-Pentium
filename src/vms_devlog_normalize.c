#include "vms_devlog_search.h"
/* IOSUBPAGD 70$-200$: repeat LOG$TRANSLATE only until NOTRAN, a physical
 * underscore, or at most ten translations. No fabricated remote UCB. */
uint32_t port_devlog_normalize(const struct port_log_system_table *t,
    struct port_devlog_state *s){
    if(!t||!s||!s->length||s->length>PORT_V37_IOC_NAME_MAX)
        return PORT_V37_IOC_BAD;
    for(unsigned i=0;i<PORT_DEVLOG_MAX_STEPS;i++){
        uint32_t v=port_devlog_remote(s->name,s->length);
        if(v!=PORT_V37_IOC_OK)return v;
        uint8_t concealed=0;
        v=port_devlog_underscore(s->name,s->length,&concealed);
        if(v==PORT_V37_IOC_LOGICAL_PENDING){
            s->concealed=concealed;s->stopped=1u;
            return PORT_V37_IOC_OK;
        }
        if(v!=PORT_V37_IOC_OK)return v;
        if(s->stopped)return PORT_V37_IOC_OK;
        v=port_devlog_attempt(t,s);
        if(v!=PORT_V37_IOC_OK)return v;
    }
    return PORT_V37_IOC_BAD; /* a cyclic or overlong logical chain */
}
