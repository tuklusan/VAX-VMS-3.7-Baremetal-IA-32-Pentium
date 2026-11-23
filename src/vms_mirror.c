#include "vms_mirror.h"
int port_mirror_make_request(uint8_t *o,unsigned cap,const uint8_t *d,unsigned n) {
    if(!o||(!d&&n)||n>=PORT_MIRROR_MAX||cap<n+1u)return -1;
    o[0]=PORT_MIRROR_CALL;
    for(unsigned i=0;i<n;i++)o[i+1]=d[i];
    return (int)n+1;
}
int port_mirror_reply(uint8_t *o,unsigned cap,const uint8_t *r,unsigned n) {
    if(!o||!r||!n||n>PORT_MIRROR_MAX||!cap)return -1;
    if(r[0]!=PORT_MIRROR_CALL){o[0]=PORT_MIRROR_ERROR;return 1;}
    if(cap<n)return -1;
    o[0]=PORT_MIRROR_OK;
    for(unsigned i=1;i<n;i++)o[i]=r[i];
    return (int)n;
}
int port_mirror_verify(const uint8_t *request,unsigned nr,
                       const uint8_t *response,unsigned ns) {
    if(!request||!response||!nr||nr>PORT_MIRROR_MAX||
       request[0]!=PORT_MIRROR_CALL||!ns||ns>PORT_MIRROR_MAX)return -1;
    if(ns==1&&response[0]==PORT_MIRROR_ERROR)return -2;
    if(ns!=nr||response[0]!=PORT_MIRROR_OK)return -1;
    for(unsigned i=1;i<nr;i++)if(request[i]!=response[i])return -1;
    return 0;
}
