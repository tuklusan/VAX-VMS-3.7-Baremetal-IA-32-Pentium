#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "vms_nsp_data.h"
#include "vms_decnet.h"
int main(void) {
    uint8_t data[600],wire[1518];
    struct port_nsp_data pkt={0};struct port_nsp_data_ack ack={0};
    struct port_decnet_short shortpkt={0};
    const uint8_t ping[4]={'P','I','N','G'};
    assert(port_nsp_data_build(data,sizeof data,PORT_NSP_DATA_MOM,
            0x7788,0x3737,0,0,1,ping,4)==15);
    const uint8_t expected[]={0x60,0x88,0x77,0x37,0x37,
        0,0x80,0,0xa0,1,0,'P','I','N','G'};
    assert(memcmp(data,expected,sizeof expected)==0);
    assert(port_nsp_data_parse(data,15,&pkt)==0);
    assert(pkt.ack_data==0&&pkt.ack_other==0&&pkt.sequence==1&&
           pkt.body_len==4&&memcmp(pkt.body,ping,4)==0);
    int n=port_decnet_build_short(wire,sizeof wire,0x401,0x402,data,15);
    assert(n==37);
    assert(port_decnet_parse_short(wire,(unsigned)n,&shortpkt)==0);
    assert(port_nsp_data_parse(shortpkt.payload,shortpkt.payload_len,&pkt)==0);
    assert(port_nsp_data_ack_build(data,sizeof data,0x3737,0x7788,0xfff,8)==9);
    assert(port_nsp_data_ack_parse(data,9,&ack)==0);
    assert(ack.destination_link==0x3737&&ack.ack_data==4095&&ack.ack_other==8);
    data[6]=0x90;assert(port_nsp_data_ack_parse(data,9,&ack)<0);data[6]=0x8f;
    data[0]=0x14;assert(port_nsp_data_ack_parse(data,9,&ack)<0);
    assert(port_nsp_data_ack_parse(data,8,&ack)<0);
    assert(port_nsp_data_build(data,15,0x60,1,1,0,0,4096,ping,4)<0);
    assert(port_nsp_data_build(data,15,0x60,1,1,0,0,1,ping,513)<0);
    assert(port_nsp_data_build(data,15,0x61,1,1,0,0,1,ping,4)<0);
    assert(port_nsp_data_parse(expected,10,&pkt)<0);
    puts("M19 NSP DATA segment / acknowledgment codec PASS");return 0;
}
