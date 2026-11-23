/* Deterministic 2-node in-memory Phase IV wire test; no host NIC required. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "vms_decnet.h"
#include "vms_nsp.h"
#include "vms_nsp_data.h"
#include "vms_nsp_link.h"
#include "vms_nsp_reliable.h"
#include "vms_session.h"
#include "vms_mirror.h"
#include "vms_mirror_server.h"
#define CLIENT 0x0401u
#define SERVER 0x0402u
#define CLINK 0x3737u
static uint8_t frame[1518];
static struct port_decnet_short route(uint16_t from,uint16_t to,
                                     const uint8_t *nsp,unsigned len) {
    struct port_decnet_short m={0};
    int n=port_decnet_build_short(frame,sizeof frame,from,to,nsp,len);
    assert(n>0);
    assert(port_decnet_parse_short(frame,(unsigned)n,&m)==0);
    assert(m.src_node==from&&m.dst_node==to);
    return m;
}
int main(void) {
    uint8_t tmp[600],req[64],session[PORT_SESSION_REQUEST_BYTES],
            ack[32], client_data[600];
    struct port_nsp_message ctrl={0};
    struct port_nsp_link client={0};
    struct port_nsp_reliable creliab;
    struct port_mirror_server server;
    struct port_nsp_data data={0};
    struct port_decnet_short r;
    uint16_t seq=0;
    port_nsp_link_reset(&client);
    port_nsp_reliable_init(&creliab);
    port_mirror_server_init(&server);
    assert(port_nsp_link_open(&client,CLIENT,SERVER,CLINK,0)==0);
    assert(port_session_build_request(session,sizeof session,25)==14);
    int n=port_nsp_build_connect(tmp,sizeof tmp,PORT_NSP_CI,CLINK,0,512,
                                  session,sizeof session);
    r=route(CLIENT,SERVER,tmp,(unsigned)n);
    assert(port_nsp_parse(r.payload,r.payload_len,&ctrl)==0);
    assert(port_mirror_server_connect(&server,SERVER,CLIENT,&ctrl)==1);
    /* Connect ACK followed by Connect Confirm. */
    n=port_nsp_build_conn_ack(tmp,sizeof tmp,CLINK);
    r=route(SERVER,CLIENT,tmp,(unsigned)n);
    assert(port_nsp_parse(r.payload,r.payload_len,&ctrl)==0);
    assert(port_nsp_link_rx(&client,SERVER,CLIENT,&ctrl)==0);
    assert(client.state==PORT_NSP_ACKED);
    const uint8_t accept_data[]={2,0,2};
    n=port_nsp_build_connect(tmp,sizeof tmp,PORT_NSP_CC,
          server.local_link,CLINK,512,accept_data,sizeof accept_data);
    r=route(SERVER,CLIENT,tmp,(unsigned)n);
    assert(port_nsp_parse(r.payload,r.payload_len,&ctrl)==0);
    assert(port_nsp_link_rx(&client,SERVER,CLIENT,&ctrl)==0);
    assert(client.state==PORT_NSP_CONNECTED);
    assert(client.remote_link==PORT_MIRROR_SERVER_LINK);
    /* Client sends function 0x00 and bytes to echo, one segment. */
    const uint8_t token[]={0,1,2,0xff,'Z','Z'};
    int req_len=port_mirror_make_request(req,sizeof req,token,sizeof token);
    assert(req_len==7);
    assert(port_nsp_tx_begin(&creliab,100,&seq)==0&&seq==1);
    int c_len=port_nsp_data_build(client_data,sizeof client_data,0x60,
          client.remote_link,CLINK,0,0,seq,req,(unsigned)req_len);
    assert(c_len==18);
    r=route(CLIENT,SERVER,client_data,(unsigned)c_len);
    int replylen=port_mirror_server_data(&server,CLIENT,r.payload,
                                         r.payload_len,100);
    assert(replylen==c_len&&server.echoed==1);
    /* Drop the original ACK/response. Client retransmits identical DATA. */
    assert(port_nsp_tx_tick(&creliab,300)==1);
    r=route(CLIENT,SERVER,client_data,(unsigned)c_len);
    assert(port_mirror_server_data(&server,CLIENT,r.payload,
                                   r.payload_len,300)==0);
    assert(server.echoed==1&&server.transport.duplicates==1);
    /* Also drop server's original DATA; its timer requests byte-identical retry. */
    assert(port_mirror_server_tick(&server,300)==1);
    r=route(SERVER,CLIENT,server.replay,server.replay_len);
    assert(port_nsp_data_parse(r.payload,r.payload_len,&data)==0);
    assert(data.ack_data==1); /* piggyback ACK acknowledges original client DATA */
    assert(port_nsp_tx_ack(&creliab,data.ack_data)==1&&!creliab.pending);
    assert(port_nsp_rx_sequence(&creliab,data.sequence)==PORT_NSP_RX_NEW);
    assert(port_mirror_verify(req,(unsigned)req_len,
                              data.body,data.body_len)==0);
    /* ACK the returned DATA: reverse-link numbering must match. */
    n=port_nsp_data_ack_build(ack,sizeof ack,PORT_MIRROR_SERVER_LINK,
                              CLINK,data.sequence,0);
    r=route(CLIENT,SERVER,ack,(unsigned)n);
    assert(port_mirror_server_ack(&server,CLIENT,r.payload,r.payload_len)==1);
    assert(server.replay_len==0);
    /* A replayed server response must not re-deliver the application data. */
    assert(port_nsp_rx_sequence(&creliab,data.sequence)==PORT_NSP_RX_DUPLICATE);
    assert(port_nsp_tx_ack(&creliab,1)==0); /* stale ACK ignored */
    /* Second correct request after both endpoints completed first exchange. */
    assert(port_nsp_tx_begin(&creliab,450,&seq)==0&&seq==2);
    c_len=port_nsp_data_build(client_data,sizeof client_data,0x60,
          client.remote_link,CLINK,1,0,seq,req,(unsigned)req_len);
    r=route(CLIENT,SERVER,client_data,(unsigned)c_len);
    replylen=port_mirror_server_data(&server,CLIENT,r.payload,
                                     r.payload_len,450);
    assert(replylen==c_len&&server.echoed==2);
    assert(port_nsp_data_parse(server.replay,server.replay_len,&data)==0);
    assert(data.sequence==2&&data.ack_data==2);
    assert(port_mirror_verify(req,(unsigned)req_len,
                              data.body,data.body_len)==0);
    printf("M24 2-node NSP CI/CA/CC/Session/Mirror, loss and retransmission PASS\n");
    return 0;
}
