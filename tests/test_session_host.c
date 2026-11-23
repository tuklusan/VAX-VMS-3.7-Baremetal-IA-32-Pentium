#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "vms_session.h"
#include "vms_nsp.h"
#include "vms_decnet.h"
int main(void) {
    uint8_t req[30],nsp[64],ether[1518];uint8_t object=0;
    struct port_nsp_message msg={0};struct port_decnet_short route={0};
    assert(port_session_build_request(req,sizeof req,25)==14);
    const uint8_t expected[14]={0,25,2,0,0,0,0,0,0,3,0,0,0,0};
    assert(memcmp(req,expected,sizeof expected)==0);
    assert(port_session_parse_request(req,14,&object)==0&&object==25);
    int n=port_nsp_build_connect(nsp,sizeof nsp,PORT_NSP_CI,0x3737,0,576,req,14);
    assert(n==23);
    int frame_len=port_decnet_build_short(ether,sizeof ether,0x401,0x402,nsp,(unsigned)n);
    assert(frame_len==45);
    assert(port_decnet_parse_short(ether,(unsigned)frame_len,&route)==0);
    assert(route.src_node==0x401&&route.dst_node==0x402);
    assert(port_nsp_parse(route.payload,route.payload_len,&msg)==0);
    assert(msg.flags==PORT_NSP_CI&&msg.optional_length==14);
    assert(port_session_parse_request(msg.optional,msg.optional_length,&object)==0&&object==25);
    req[9]=4;assert(port_session_parse_request(req,14,&object)!=0);
    req[9]=3;req[13]=1;assert(port_session_parse_request(req,14,&object)!=0);
    assert(port_session_parse_request(req,13,&object)!=0);
    assert(port_session_build_request(req,13,25)!=0);
    assert(port_session_build_request(req,30,0)!=0);
    puts("M18 NSP Session Control numbered-object request PASS");
    return 0;
}
