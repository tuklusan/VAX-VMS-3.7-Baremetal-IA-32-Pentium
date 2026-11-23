#include "vms_devlog_search.h"
/* IOSUBPAGD 70$-110$: call source LOG$TRANSLATE through the small SYSTEM
 * table, preserving the original 10-attempt bound. The only public
 * bootstrap names are SYS$DISK and SYS$SYSDEVICE. */
uint32_t port_devlog_attempt(const struct port_log_system_table *t,
    struct port_devlog_state *s){
    if(!t||!s||s->stopped||s->attempts>=PORT_DEVLOG_MAX_STEPS)
        return PORT_V37_IOC_BAD;
    size_t prefix=0;
    if(port_devlog_colon(s->name,s->length,&prefix)!=PORT_V37_IOC_OK)
        return PORT_V37_IOC_BAD;
    uint8_t next[PORT_DEVLOG_NAME_CAP]={0};
    struct port_log_result result={0};
    const uint32_t r=port_log_translate(t,s->name,prefix,0u,next,
        sizeof next,&result);
    if((r!=PORT_LOG_OK&&r!=PORT_LOG_NOTRAN)||
       result.bytes==0u||result.bytes>PORT_V37_IOC_NAME_MAX)
        return PORT_V37_IOC_BAD;
    struct port_devlog_state tmp=*s;
    for(unsigned i=0;i<result.bytes;i++)tmp.name[i]=(char)next[i];
    tmp.name[result.bytes]=0;tmp.length=result.bytes;
    tmp.attempts++;
    tmp.last_notran=(uint8_t)(r==PORT_LOG_NOTRAN);
    tmp.stopped=(uint8_t)(r==PORT_LOG_NOTRAN);
    *s=tmp;
    return PORT_V37_IOC_OK;
}
