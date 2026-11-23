#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "vms_nsp.h"
#include "vms_decnet.h"
int main(void) {
    uint8_t p[576], snap[576];struct port_nsp_message msg={0};
    uint8_t session[]={0,3,3,0,0};
    assert(port_nsp_build_connect(p,sizeof p,PORT_NSP_CI,0x1234,0,576,session,sizeof session)==14);
    assert(memcmp(p,(uint8_t[]){0x18,0,0,0x34,0x12,0,0,0x40,2,0,3,3,0,0},14)==0);
    assert(port_nsp_parse(p,14,&msg)==0);
    assert(msg.flags==PORT_NSP_CI&&msg.source_link==0x1234&&msg.segment_size==576);
    assert(msg.optional_length==sizeof session&&memcmp(msg.optional,session,sizeof session)==0);
    assert(port_nsp_parse(p,8,&msg)==PORT_NSP_EFORMAT);
    assert(port_nsp_build_connect(p,sizeof p,PORT_NSP_CC,0x4444,0x1234,1024,0,0)==9);
    assert(port_nsp_parse(p,9,&msg)==0&&msg.destination_link==0x1234&&msg.source_link==0x4444);
    p[5]=0x0c;assert(port_nsp_parse(p,9,&msg)==PORT_NSP_EFORMAT);
    p[5]=0;
    assert(port_nsp_build_conn_ack(p,sizeof p,0x1234)==3);
    assert(port_nsp_parse(p,3,&msg)==0&&msg.flags==PORT_NSP_CA);
    assert(port_nsp_parse(p,2,&msg)==PORT_NSP_EFORMAT);
    assert(port_nsp_build_disconnect(p,sizeof p,PORT_NSP_DI,0x1234,0x4444,42)==7);
    assert(port_nsp_parse(p,7,&msg)==0&&msg.reason==42);
    assert(port_nsp_build_disconnect(p,sizeof p,PORT_NSP_DC,0x1234,0x4444,42)==7);
    assert(port_nsp_parse(p,7,&msg)==0);
    p[7]=1;assert(port_nsp_parse(p,8,&msg)==PORT_NSP_EFORMAT);
    p[0]=0xff;assert(port_nsp_parse(p,8,&msg)==PORT_NSP_ENOTSUP);
    memset(p,0,sizeof p);memcpy(snap,p,sizeof p);
    assert(port_nsp_build_connect(p,8,PORT_NSP_CI,7,0,512,0,0)<0);
    assert(memcmp(p,snap,sizeof p)==0); /* no partial writes on error */
    /* Wire-through: DECnet short routing payload -> NSP decoder. */
    uint8_t frame[1518];struct port_decnet_short shortmsg;
    assert(port_nsp_build_connect(p,sizeof p,PORT_NSP_CI,0x1234,0,576,0,0)==9);
    int frame_len=port_decnet_build_short(frame,sizeof frame,0x0402,0x0401,p,9);
    assert(frame_len==31);
    assert(port_decnet_parse_short(frame,(unsigned)frame_len,&shortmsg)==0);
    assert(shortmsg.dst_node==0x0401&&shortmsg.src_node==0x0402);
    assert(port_nsp_parse(shortmsg.payload,shortmsg.payload_len,&msg)==0);
    assert(msg.flags==PORT_NSP_CI&&msg.source_link==0x1234);
    assert(port_nsp_parse(p,577,&msg)==PORT_NSP_EFORMAT);
    assert(port_nsp_parse(0,7,&msg)==PORT_NSP_EINVAL);
    assert(port_nsp_build_disconnect(p,7,PORT_NSP_DI,0,0,1)==7);
    puts("M16 NSP bounded control wire format PASS");return 0;
}
