#include "vms_devlog_search.h"
/* Source 150$-180$: a single colon terminates the name to translate. An
 * RMS file specification after ':' is not a device lookup at this stage. */
uint32_t port_devlog_colon(const char *s,size_t n,size_t *part){
    if(!s||!part||!n||n>PORT_V37_IOC_NAME_MAX)return PORT_V37_IOC_BAD;
    size_t i=0;
    while(i<n&&s[i]!=':')i++;
    if(i==0u)return PORT_V37_IOC_BAD;
    if(i<n&&i+1u<n)return PORT_V37_IOC_BAD;
    *part=i;
    return PORT_V37_IOC_OK;
}
