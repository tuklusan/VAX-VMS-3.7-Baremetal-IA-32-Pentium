#include "vms_nsp.h"
static uint16_t rd16(const uint8_t *p) {
    return (uint16_t)(p[0] | ((uint16_t)p[1]<<8));
}
static void wr16(uint8_t *p,uint16_t v) {
    p[0]=(uint8_t)v;p[1]=(uint8_t)(v>>8);
}
int port_nsp_parse(const uint8_t *p,unsigned n,struct port_nsp_message *m) {
    if(!p||!m)return PORT_NSP_EINVAL;
    if(n<1||n>PORT_NSP_MAX_SEGMENT)return PORT_NSP_EFORMAT;
    /* Reject reserved bit patterns rather than accidentally accepting data. */
    uint8_t f=p[0];
    switch(f) {
    case PORT_NSP_CA:
        if(n!=3||rd16(p+1)==0)return PORT_NSP_EFORMAT;
        m->destination_link=rd16(p+1);
        m->source_link=0;m->services=0;m->info=0;
        m->segment_size=0;m->reason=0;
        m->optional=0;m->optional_length=0;
        break;
    case PORT_NSP_CI:
    case PORT_NSP_RCI:
    case PORT_NSP_CC:
        if(n<9)return PORT_NSP_EFORMAT;
        if(rd16(p+3)==0||rd16(p+7)==0)return PORT_NSP_EFORMAT;
        if(f==PORT_NSP_CC&&rd16(p+1)==0)return PORT_NSP_EFORMAT;
        if(f!=PORT_NSP_CC&&rd16(p+1)!=0)return PORT_NSP_EFORMAT;
        if((p[5]&0x0cu)==0x0cu)return PORT_NSP_EFORMAT;
        m->destination_link=rd16(p+1);m->source_link=rd16(p+3);
        m->services=p[5];m->info=p[6];m->segment_size=rd16(p+7);
        m->reason=0;m->optional=p+9;m->optional_length=n-9;
        break;
    case PORT_NSP_DI:
    case PORT_NSP_DC:
        if(n<7||(f==PORT_NSP_DC&&n!=7))return PORT_NSP_EFORMAT;
        /* DI may have optional disconnect user data (handled by future
         * Session Control layer). DC contains exactly a reason field. */
        m->destination_link=rd16(p+1);m->source_link=rd16(p+3);
        m->reason=rd16(p+5);m->services=0;m->info=0;
        m->segment_size=0;m->optional=n>7?p+7:0;
        m->optional_length=n-7;
        break;
    default:
        return PORT_NSP_ENOTSUP;
    }
    m->flags=f;
    return PORT_NSP_OK;
}
int port_nsp_build_connect(uint8_t *p,unsigned cap,uint8_t flags,
                           uint16_t source,uint16_t destination,
                           uint16_t segsize,const uint8_t *session,
                           unsigned session_len) {
    if(!p||!(flags==PORT_NSP_CI||flags==PORT_NSP_RCI||flags==PORT_NSP_CC)||
       !source||!segsize||(flags==PORT_NSP_CC?!destination:destination!=0)||
       (session_len&&!session)||session_len>PORT_NSP_MAX_SEGMENT-9u||
       cap<9u+session_len)return PORT_NSP_EINVAL;
    p[0]=flags;wr16(p+1,destination);wr16(p+3,source);
    p[5]=PORT_NSP_NO_FLOW_CONTROL;p[6]=0;wr16(p+7,segsize);
    for(unsigned i=0;i<session_len;i++)p[9+i]=session[i];
    return (int)(9u+session_len);
}
int port_nsp_build_conn_ack(uint8_t *p,unsigned cap,uint16_t destination) {
    if(!p||cap<3||!destination)return PORT_NSP_EINVAL;
    p[0]=PORT_NSP_CA;wr16(p+1,destination);return 3;
}
int port_nsp_build_disconnect(uint8_t *p,unsigned cap,uint8_t flags,
                              uint16_t destination,uint16_t source,
                              uint16_t reason) {
    if(!p||cap<7||!(flags==PORT_NSP_DI||flags==PORT_NSP_DC))
        return PORT_NSP_EINVAL;
    p[0]=flags;wr16(p+1,destination);wr16(p+3,source);
    wr16(p+5,reason);return 7;
}
