#include "vms_devlog_search.h"
/* Source 120$-150$: one underscore escapes logical translation; the
 * second sets CONCEALED. Keep both for the physical lookup. */
uint32_t port_devlog_underscore(const char *s,size_t n,uint8_t *concealed){
    if(!s||!concealed||!n)return PORT_V37_IOC_BAD;
    unsigned k=0;
    while(k<n&&s[k]=='_'&&k<3u)k++;
    if(k>2u||k==n)return PORT_V37_IOC_BAD;
    *concealed=(uint8_t)(k==2u);
    return k?PORT_V37_IOC_LOGICAL_PENDING:PORT_V37_IOC_OK;
}
