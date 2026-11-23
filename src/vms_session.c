#include "vms_session.h"
int port_session_build_request(uint8_t *p,unsigned cap,uint8_t object) {
    if(!p||cap<PORT_SESSION_REQUEST_BYTES||!object)return PORT_SESSION_EFORMAT;
    p[0]=0;          /* destination by object number */
    p[1]=object;
    p[2]=2;          /* source identifier in type-2 form */
    for(unsigned i=3;i<9;i++)p[i]=0; /* zero UID/GID/name */
    p[9]=PORT_SESSION_MENUVER; /* access and user data fields present */
    p[10]=0;         /* username length */
    p[11]=0;         /* password length */
    p[12]=0;         /* account length */
    p[13]=0;         /* optional connect data length */
    return PORT_SESSION_REQUEST_BYTES;
}
int port_session_parse_request(const uint8_t *p,unsigned len,uint8_t *object) {
    if(!p||!object||len!=PORT_SESSION_REQUEST_BYTES||
       p[0]!=0||p[1]==0||p[2]!=2||p[9]!=PORT_SESSION_MENUVER)
        return PORT_SESSION_EFORMAT;
    for(unsigned i=3;i<9;i++)if(p[i]!=0)return PORT_SESSION_EFORMAT;
    for(unsigned i=10;i<14;i++)if(p[i]!=0)return PORT_SESSION_EFORMAT;
    *object=p[1];return PORT_SESSION_OK;
}
