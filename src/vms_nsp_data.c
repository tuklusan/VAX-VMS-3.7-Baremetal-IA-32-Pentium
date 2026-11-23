#include "vms_nsp_data.h"
static uint16_t get16(const uint8_t *p){return (uint16_t)(p[0]|((uint16_t)p[1]<<8));}
static void put16(uint8_t *p,uint16_t v){p[0]=(uint8_t)v;p[1]=(uint8_t)(v>>8);}
static int goodflags(uint8_t f){return f==0||f==0x20||f==0x40||f==0x60;}
static int goodack(uint16_t a,uint16_t b) {
    return (a&0xf000u)==0x8000u && (b&0xf000u)==0xa000u;
}
int port_nsp_data_build(uint8_t *p,unsigned cap,uint8_t flags,
                        uint16_t dst,uint16_t src,uint16_t ack,
                        uint16_t other,uint16_t seq,const uint8_t *body,
                        unsigned n) {
    if(!p||!goodflags(flags)||!dst||!src||ack>0xfff||other>0xfff||
       seq>0xfff||(!body&&n)||n>PORT_NSP_DATA_MAX_BYTES||cap<11u+n)
        return -1;
    p[0]=flags;put16(p+1,dst);put16(p+3,src);
    put16(p+5,(uint16_t)(0x8000u|ack));
    put16(p+7,(uint16_t)(0xa000u|other));
    put16(p+9,seq);
    for(unsigned i=0;i<n;i++)p[11+i]=body[i];
    return (int)(11u+n);
}
int port_nsp_data_parse(const uint8_t *p,unsigned n,struct port_nsp_data *o) {
    if(!p||!o||n<11||n>11u+PORT_NSP_DATA_MAX_BYTES||!goodflags(p[0]))
        return -1;
    uint16_t a=get16(p+5),b=get16(p+7),s=get16(p+9);
    if(!get16(p+1)||!get16(p+3)||!goodack(a,b)||s>0xfffu)return -1;
    o->flags=p[0];o->destination_link=get16(p+1);o->source_link=get16(p+3);
    o->ack_data=a&0xfffu;o->ack_other=b&0xfffu;
    o->sequence=s;o->body=p+11;o->body_len=n-11;
    return 0;
}
int port_nsp_data_ack_build(uint8_t *p,unsigned cap,uint16_t dst,
                             uint16_t src,uint16_t ack,uint16_t other) {
    if(!p||cap<9||!dst||!src||ack>0xfffu||other>0xfffu)return -1;
    p[0]=PORT_NSP_DATA_ACK;put16(p+1,dst);put16(p+3,src);
    put16(p+5,(uint16_t)(0x8000u|ack));
    put16(p+7,(uint16_t)(0xa000u|other));return 9;
}
int port_nsp_data_ack_parse(const uint8_t *p,unsigned n,
                             struct port_nsp_data_ack *o) {
    if(!p||!o||n!=9||p[0]!=PORT_NSP_DATA_ACK||
       !get16(p+1)||!get16(p+3)||
       !goodack(get16(p+5),get16(p+7)))return -1;
    o->destination_link=get16(p+1);o->source_link=get16(p+3);
    o->ack_data=get16(p+5)&0xfffu;o->ack_other=get16(p+7)&0xfffu;
    return 0;
}
