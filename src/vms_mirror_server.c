#include "vms_mirror_server.h"
#include "vms_session.h"
#include "vms_mirror.h"
static int valid_node(uint16_t n){return n&&(n&0x03ffu)!=0;}
void port_mirror_server_init(struct port_mirror_server *s) {
    if(!s)return;
    *s=(struct port_mirror_server){0};
    port_nsp_reliable_init(&s->transport);
}
int port_mirror_server_matches(const struct port_mirror_server *s,
                               uint16_t peer,uint16_t dst,uint16_t src) {
    return s&&s->active&&peer==s->peer_node&&
           dst==s->local_link&&src==s->peer_link;
}
int port_mirror_server_connect(struct port_mirror_server *s,
                               uint16_t local,uint16_t peer,
                               const struct port_nsp_message *ci) {
    if(!s||!ci||!valid_node(local)||!valid_node(peer)||local==peer||
       (ci->flags!=PORT_NSP_CI&&ci->flags!=PORT_NSP_RCI)||
       ci->destination_link||!ci->source_link||!ci->segment_size) {
        if(s)++s->rejects;
        return -1;
    }
    if(s->active) {
        if(s->local_node==local&&s->peer_node==peer&&
           s->peer_link==ci->source_link) {
            ++s->dup_connects;return 2;
        }
        ++s->rejects;return PORT_MIRROR_SERVER_BUSY;
    }
    uint8_t object=0;
    if(port_session_parse_request(ci->optional,ci->optional_length,&object)<0||
       object!=PORT_SESSION_MIRROR_OBJECT) {
        ++s->rejects;return -1;
    }
    s->active=1;s->local_node=local;s->peer_node=peer;
    s->local_link=PORT_MIRROR_SERVER_LINK;s->peer_link=ci->source_link;
    s->replay_len=0;
    port_nsp_reliable_init(&s->transport);
    ++s->accepts;return 1;
}
int port_mirror_server_ack(struct port_mirror_server *s,uint16_t peer,
                           const uint8_t *bytes,unsigned n) {
    struct port_nsp_data_ack a;
    if(!s||!s->active||!bytes||
       port_nsp_data_ack_parse(bytes,n,&a)<0||
       !port_mirror_server_matches(s,peer,a.destination_link,a.source_link))
        return -1;
    int rc=port_nsp_tx_ack(&s->transport,a.ack_data);
    if(rc==1)s->replay_len=0;
    return rc;
}
int port_mirror_server_data(struct port_mirror_server *s,uint16_t peer,
                            const uint8_t *bytes,unsigned n,uint32_t now) {
    struct port_nsp_data data;
    if(!s||!s->active||!bytes||port_nsp_data_parse(bytes,n,&data)<0||
       !port_mirror_server_matches(s,peer,data.destination_link,data.source_link)||
       data.flags!=PORT_NSP_DATA_MOM||data.body_len==0) {
        if(s)++s->bad_data;
        return -1;
    }
    /* ACK on a DATA packet applies to a previously transmitted reply. */
    if(s->transport.pending&&data.ack_data==s->transport.pending_seq) {
        (void)port_nsp_tx_ack(&s->transport,data.ack_data);
        s->replay_len=0;
    }
    /* Do not advance the receive sequence if a new response cannot be
       queued. Duplicates are legal even while the previous TX is busy. */
    if(data.sequence==s->transport.next_rx&&s->transport.pending)
        return PORT_MIRROR_SERVER_BUSY;
    int rc=port_nsp_rx_sequence(&s->transport,data.sequence);
    if(rc==PORT_NSP_RX_DUPLICATE)return 0;
    if(rc!=PORT_NSP_RX_NEW) {
        ++s->bad_data;return PORT_MIRROR_SERVER_ORDER;
    }
    uint8_t reflected[PORT_NSP_DATA_MAX_BYTES];
    int payload=port_mirror_reply(reflected,sizeof reflected,
                                  data.body,data.body_len);
    if(payload<0) {++s->bad_data;return -1;}
    uint16_t seq;
    if(port_nsp_tx_begin(&s->transport,now,&seq)<0)return -1;
    int len=port_nsp_data_build(s->replay,sizeof s->replay,PORT_NSP_DATA_MOM,
            s->peer_link,s->local_link,s->transport.last_rx,0,seq,
            reflected,(unsigned)payload);
    if(len<=0) {port_nsp_tx_cancel(&s->transport);++s->bad_data;return -1;}
    s->replay_len=(unsigned)len;
    ++s->echoed;return len;
}
int port_mirror_server_tick(struct port_mirror_server *s,uint32_t now) {
    if(!s||!s->active)return 0;
    int rc=port_nsp_tx_tick(&s->transport,now);
    if(rc==-1)s->replay_len=0;
    return rc;
}
void port_mirror_server_close(struct port_mirror_server *s) {
    if(!s)return;
    s->active=0;s->replay_len=0;
    port_nsp_tx_cancel(&s->transport);
}
