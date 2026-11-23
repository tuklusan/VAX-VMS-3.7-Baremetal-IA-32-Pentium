#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "vms_filerwio.h"
static int failures;
#define CHECK(x) do{if(!(x)){fprintf(stderr,"FAIL %s:%d %s\n",__FILE__,__LINE__,#x);failures++;}}while(0)
struct case_data {uint32_t queue,channels,fn,lbn,bytes;uint16_t done;int calls;};
static uint32_t transfer(void *p,uint32_t ch,uint32_t fn,void *buffer,
                         uint32_t bytes,uint32_t lbn,
                         struct port_filerwio_iosb *iosb){
    struct case_data *c=p; c->channels=ch;c->fn=fn;c->lbn=lbn;
    c->bytes=bytes;c->calls++;
    if(buffer && bytes)memset(buffer,0x37,bytes);
    iosb->status=c->done;iosb->count=(uint16_t)bytes;
    return c->queue;
}
int main(void){
    struct case_data c={1u,0u,0u,0u,0u,1u,0};
    struct port_filerwio_service service={transfer,&c};
    uint8_t sectors[1536];memset(sectors,0,sizeof sectors);
    CHECK(port_filerwio_rdwr_lbn(&service,0x36,0x109u,sectors,0x20u,
                                  sizeof sectors)==1u);
    CHECK(c.channels==0x36u && c.fn==0x20u && c.lbn==0x109u &&
          c.bytes==1536u && c.calls==1 && sectors[1535]==0x37);
    c.done=0x10u;
    CHECK(port_filerwio_rdwr_lbn(&service,0x36,0,sectors,0x20,512)==0x10u);
    c.queue=0x12u;c.done=1u;
    CHECK(port_filerwio_rdwr_lbn(&service,0x36,0,sectors,0x20,512)==0x12u);
    CHECK(port_filerwio_rdwr_lbn(0,0,0,0,0,512)==0u);
    CHECK(port_filerwio_rdwr_lbn(&service,0,0,0,0,512)==0u);
    printf("FILERWIO source QIOW/IOSB: %s\n",failures?"FAIL":"PASS");
    return !!failures;
}
