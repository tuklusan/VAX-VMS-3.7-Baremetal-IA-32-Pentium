#include "vms_dq_ioc_write.h"
#include <stdio.h>
#include <string.h>
struct backend {
    uint8_t media[2][512];uint32_t order[4];unsigned writes,calls;
    uint32_t fail_lbn;
};
static uint32_t write_media(void *ctx,uint32_t lbn,const uint8_t data[512]){
    struct backend *b=ctx;
    if(b->calls>=4u)return PORT_DQ_IOCW_IO;
    b->order[b->calls++]=lbn;
    if(lbn>=2u||lbn==b->fail_lbn||b->writes>=2u)return PORT_DQ_IOCW_IO;
    memcpy(b->media[lbn],data,512u);b->writes++;
    return PORT_DQ_IOCW_OK;
}
int main(void){
    struct backend b={.fail_lbn=UINT32_MAX};struct port_dq_ioc_write_queue q;
    struct port_dq_ioc_write_iosb iosb;uint8_t first[512],second[512];
    for(unsigned i=0;i<512u;i++){first[i]=(uint8_t)(i^0xA5u);second[i]=(uint8_t)(i^0x5Au);}
    if(port_dq_ioc_write_init(&q,write_media,&b)!=PORT_DQ_IOCW_OK||
       port_dq_ioc_write_prepare(&q,1u,1u,first,sizeof first)!=PORT_DQ_IOCW_OK||
       port_dq_ioc_write_prepare(&q,2u,0u,second,sizeof second)!=PORT_DQ_IOCW_OK||
       port_dq_ioc_write_set_priority(&q,1u,2u)!=PORT_DQ_IOCW_OK||
       port_dq_ioc_write_set_priority(&q,2u,1u)!=PORT_DQ_IOCW_OK||
       port_dq_ioc_write_enqueue(&q,1u)!=PORT_DQ_IOCW_OK||
       port_dq_ioc_write_enqueue(&q,2u)!=PORT_DQ_IOCW_OK)return 1;
    memset(first,0,sizeof first);memset(second,0,sizeof second);
    if(port_dq_ioc_write_service(&q,&iosb)!=PORT_DQ_IOCW_OK||
       iosb.tag!=2u||iosb.status!=PORT_DQ_IOCW_OK||iosb.bytes!=512u||
       port_dq_ioc_write_service(&q,&iosb)!=PORT_DQ_IOCW_OK||
       iosb.tag!=1u||iosb.status!=PORT_DQ_IOCW_OK||iosb.bytes!=512u||
       b.writes!=2u||b.order[0]!=0u||b.order[1]!=1u||
       q.completed!=2u||q.failed!=0u)return 2;
    for(unsigned i=0;i<512u;i++)
        if(b.media[1][i]!=(uint8_t)(i^0xA5u)||
           b.media[0][i]!=(uint8_t)(i^0x5Au))return 3;
    memset(&b,0,sizeof b);b.fail_lbn=7u;
    if(port_dq_ioc_write_init(&q,write_media,&b)!=PORT_DQ_IOCW_OK||
       port_dq_ioc_write_prepare(&q,7u,7u,first,sizeof first)!=PORT_DQ_IOCW_OK||
       port_dq_ioc_write_prepare(&q,8u,0u,second,sizeof second)!=PORT_DQ_IOCW_OK||
       port_dq_ioc_write_set_priority(&q,7u,2u)!=PORT_DQ_IOCW_OK||
       port_dq_ioc_write_set_priority(&q,8u,1u)!=PORT_DQ_IOCW_OK||
       port_dq_ioc_write_enqueue(&q,7u)!=PORT_DQ_IOCW_OK||
       port_dq_ioc_write_enqueue(&q,8u)!=PORT_DQ_IOCW_OK)return 4;
    if(port_dq_ioc_write_service(&q,&iosb)!=PORT_DQ_IOCW_OK||
       iosb.tag!=8u||iosb.status!=PORT_DQ_IOCW_OK||iosb.bytes!=512u||
       port_dq_ioc_write_service(&q,&iosb)!=PORT_DQ_IOCW_IO||
       iosb.tag!=7u||iosb.status!=PORT_DQ_IOCW_IO||iosb.bytes!=0u||
       b.calls!=2u||b.order[0]!=0u||b.order[1]!=7u||
       b.writes!=1u||q.submitted!=2u||q.completed!=1u||q.failed!=1u)
        return 5;
    for(unsigned i=0;i<512u;i++)if(b.media[0][i]!=0u)return 6;
    memset(&b,0,sizeof b);b.fail_lbn=UINT32_MAX;
    if(port_dq_ioc_write_init(&q,write_media,&b)!=PORT_DQ_IOCW_OK||
       port_dq_ioc_write_prepare(&q,9u,1u,first,sizeof first)!=PORT_DQ_IOCW_OK||
       port_dq_ioc_write_prepare(&q,10u,0u,second,sizeof second)!=PORT_DQ_IOCW_OK||
       port_dq_ioc_write_set_priority(&q,9u,4u)!=PORT_DQ_IOCW_OK||
       port_dq_ioc_write_set_priority(&q,10u,4u)!=PORT_DQ_IOCW_OK||
       port_dq_ioc_write_enqueue(&q,9u)!=PORT_DQ_IOCW_OK||
       port_dq_ioc_write_enqueue(&q,10u)!=PORT_DQ_IOCW_OK||
       port_dq_ioc_write_service(&q,&iosb)!=PORT_DQ_IOCW_OK||
       iosb.tag!=9u||port_dq_ioc_write_service(&q,&iosb)!=PORT_DQ_IOCW_OK||
       iosb.tag!=10u)return 7;
    puts("DQ IOC WRITEPBLK source priority queue, owned payload and failure completion PASS");
    return 0;
}
