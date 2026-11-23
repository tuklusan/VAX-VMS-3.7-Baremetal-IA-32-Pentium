#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "vms_mirror_server.h"
#include "vms_mirror.h"
#include "vms_session.h"
#include "vms_nsp.h"
#include "vms_decnet.h"
int main(void) {
    uint8_t b[600],wire[1518],request[128],ack[16];
    uint8_t sess[PORT_SESSION_REQUEST_BYTES];
    struct port_mirror_server srv;
    struct port_nsp_message ci={0};
    struct port_decnet_short routed;
    struct port_nsp_data reflected;
    port_mirror_server_init(&srv);
    assert(port_session_build_request(sess,sizeof sess,25)==14);
    int n=port_nsp_build_connect(b,sizeof b,PORT_NSP_CI,0x3737,0,512,sess,14);
    assert(n==23);
    int wl=port_decnet_build_short(wire,sizeof wire,0x0402,0x0401,b,n);
    assert(wl>0&&port_decnet_parse_short(wire,wl,&routed)==0);
    assert(port_nsp_parse(routed.payload,routed.payload_len,&ci)==0);
    assert(port_mirror_server_connect(&srv,0x0401,routed.src_node,&ci)==1);
    assert(srv.active&&srv.local_link==PORT_MIRROR_SERVER_LINK);
    assert(port_mirror_server_connect(&srv,0x0401,0x0402,&ci)==2);
    assert(srv.dup_connects==1);
    assert(port_mirror_server_connect(&srv,0x0401,0x0403,&ci)==PORT_MIRROR_SERVER_BUSY);
    assert(port_mirror_server_matches(&srv,0x0402,PORT_MIRROR_SERVER_LINK,0x3737));
    assert(!port_mirror_server_matches(&srv,0x0403,PORT_MIRROR_SERVER_LINK,0x3737));
    const uint8_t payload[]={'T','E','S','T',0xff};
    int bytes=port_mirror_make_request(request,sizeof request,payload,sizeof payload);
    assert(bytes==6);
    n=port_nsp_data_build(b,sizeof b,0x60,PORT_MIRROR_SERVER_LINK,
                          0x3737,0,0,1,request,(unsigned)bytes);
    assert(n==17);
    int m=port_mirror_server_data(&srv,0x0402,b,n,100);
    assert(m==17&&srv.echoed==1);
    assert(port_nsp_data_parse(srv.replay,srv.replay_len,&reflected)==0);
    assert(reflected.destination_link==0x3737&&
           reflected.source_link==PORT_MIRROR_SERVER_LINK);
    assert(reflected.sequence==1&&reflected.ack_data==1);
    assert(port_mirror_verify(request,(unsigned)bytes,
                               reflected.body,reflected.body_len)==0);
    /* Lost ACK: same client frame must not trigger duplicate reply. */
    assert(port_mirror_server_data(&srv,0x0402,b,n,200)==0);
    assert(srv.echoed==1&&srv.transport.duplicates==1);
    assert(port_mirror_server_data(&srv,0x0403,b,n,200)<0);
    assert(port_mirror_server_tick(&srv,250)==0);
    assert(port_mirror_server_tick(&srv,300)==1);
    assert(srv.transport.retries==1);
    assert(port_nsp_data_ack_build(ack,sizeof ack,PORT_MIRROR_SERVER_LINK,
                                  0x3737,1,0)==9);
    assert(port_mirror_server_ack(&srv,0x0403,ack,9)<0);
    assert(port_mirror_server_ack(&srv,0x0402,ack,9)==1);
    assert(!srv.transport.pending&&!srv.replay_len);
    assert(port_mirror_server_ack(&srv,0x0402,ack,9)==0);
    /* Invalid sequence and unsupported object don't consume resources. */
    n=port_nsp_data_build(b,sizeof b,0x60,PORT_MIRROR_SERVER_LINK,
                          0x3737,0,0,4,request,(unsigned)bytes);
    assert(port_mirror_server_data(&srv,0x0402,b,n,500)==PORT_MIRROR_SERVER_ORDER);
    assert(srv.echoed==1);
    port_mirror_server_close(&srv);
    assert(!srv.active);
    assert(port_session_build_request(sess,sizeof sess,17)==14);
    n=port_nsp_build_connect(b,sizeof b,PORT_NSP_CI,0x3737,0,512,sess,14);
    assert(port_nsp_parse(b,(unsigned)n,&ci)==0);
    assert(port_mirror_server_connect(&srv,0x0401,0x0402,&ci)<0&&!srv.active);
    puts("M22 inbound object-25 NSP Session/Mirror full-codec link PASS");
    return 0;
}
