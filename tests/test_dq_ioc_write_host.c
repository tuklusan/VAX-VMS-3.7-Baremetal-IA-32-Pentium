#include "vms_dq_ioc_write.h"
#include <stdio.h>
#include <string.h>
struct backend { uint8_t media[2][512];uint32_t order[2];unsigned writes; };
static uint32_t write_media(void *ctx,uint32_t lbn,const uint8_t data[512]){
    struct backend *b=ctx;
    if(lbn>=2u||b->writes>=2u)return PORT_DQ_IOCW_IO;
    b->order[b->writes]=lbn;memcpy(b->media[lbn],data,512u);b->writes++;
    return PORT_DQ_IOCW_OK;
}
int main(void){
    struct backend b={0};struct port_dq_ioc_write_queue q;
    struct port_dq_ioc_write_iosb iosb;uint8_t first[512],second[512];
    for(unsigned i=0;i<512u;i++){first[i]=(uint8_t)(i^0xA5u);second[i]=(uint8_t)(i^0x5Au);}
    if(port_dq_ioc_write_init(&q,write_media,&b)!=PORT_DQ_IOCW_OK||
       port_dq_ioc_write_prepare(&q,1u,1u,first,sizeof first)!=PORT_DQ_IOCW_OK||
       port_dq_ioc_write_prepare(&q,2u,0u,second,sizeof second)!=PORT_DQ_IOCW_OK||
       port_dq_ioc_write_enqueue(&q,1u)!=PORT_DQ_IOCW_OK||
       port_dq_ioc_write_enqueue(&q,2u)!=PORT_DQ_IOCW_OK)return 1;
    memset(first,0,sizeof first);memset(second,0,sizeof second);
    if(port_dq_ioc_write_service(&q,&iosb)!=PORT_DQ_IOCW_OK||
       iosb.tag!=1u||iosb.status!=PORT_DQ_IOCW_OK||iosb.bytes!=512u||
       port_dq_ioc_write_service(&q,&iosb)!=PORT_DQ_IOCW_OK||
       iosb.tag!=2u||iosb.status!=PORT_DQ_IOCW_OK||iosb.bytes!=512u||
       b.writes!=2u||b.order[0]!=1u||b.order[1]!=0u||
       q.completed!=2u||q.failed!=0u)return 2;
    for(unsigned i=0;i<512u;i++)
        if(b.media[1][i]!=(uint8_t)(i^0xA5u)||
           b.media[0][i]!=(uint8_t)(i^0x5Au))return 3;
    puts("DQ IOC WRITEPBLK FIFO ordering, owned payload, IOSB and completion PASS");
    return 0;
}
