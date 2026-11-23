#include "vms_devlog_search.h"
/* Source SEARCHDEV input R4,R5: freeze final spelling, length, and source
 * SS$_CONCEALED outcome separately from the physical success status. */
uint32_t port_devlog_final(const struct port_devlog_state *s,
    struct port_devlog_trace *out){
    if(!s||!out||!s->stopped||!s->length||
       s->length>PORT_V37_IOC_NAME_MAX||
       s->attempts>PORT_DEVLOG_MAX_STEPS)return PORT_V37_IOC_BAD;
    struct port_devlog_trace v={0};
    for(size_t i=0;i<s->length;i++)v.physical[i]=s->name[i];
    v.physical[s->length]=0;
    v.length=(uint16_t)s->length;
    v.translations=s->attempts;
    v.concealed=s->concealed;
    v.last_notran=s->last_notran;
    *out=v;
    return PORT_V37_IOC_OK;
}
