#include "vms_nsp_link.h"
static int valid_node(uint16_t x) {return x!=0&&(x&1023u)!=0;}
void port_nsp_link_reset(struct port_nsp_link *l) {
    if(!l)return;
    l->state=PORT_NSP_CLOSED;
    l->local_node=0;l->remote_node=0;
    l->local_link=0;l->remote_link=0;l->last_reason=0;
    l->attempts=0;l->last_tx_tick=0;
    l->received=0;l->rejected=0;l->timeouts=0;
}
int port_nsp_link_open(struct port_nsp_link *l,uint16_t local,
                       uint16_t remote,uint16_t link_id,uint32_t now) {
    if(!l||!valid_node(local)||!valid_node(remote)||local==remote||
       !link_id)return PORT_NSP_EINVAL;
    if(l->state!=PORT_NSP_CLOSED&&l->state!=PORT_NSP_REFUSED&&
       l->state!=PORT_NSP_TIMED_OUT)return PORT_NSP_EINVAL;
    port_nsp_link_reset(l);
    l->local_node=local;l->remote_node=remote;l->local_link=link_id;
    l->state=PORT_NSP_CI_SENT;l->last_tx_tick=now;l->attempts=1;
    return PORT_NSP_OK;
}
int port_nsp_link_rx(struct port_nsp_link *l,uint16_t src,uint16_t dst,
                     const struct port_nsp_message *m) {
    if(!l||!m)return PORT_NSP_EINVAL;
    if(l->state==PORT_NSP_CLOSED||l->state==PORT_NSP_REFUSED||
       l->state==PORT_NSP_TIMED_OUT||src!=l->remote_node||
       dst!=l->local_node||m->destination_link!=l->local_link) {
        ++l->rejected;return PORT_NSP_EINVAL;
    }
    if(m->flags==PORT_NSP_CA && l->state==PORT_NSP_CI_SENT) {
        l->state=PORT_NSP_ACKED;
    } else if(m->flags==PORT_NSP_CC &&
              (l->state==PORT_NSP_CI_SENT||l->state==PORT_NSP_ACKED) &&
              m->source_link!=0) {
        l->remote_link=m->source_link;
        l->state=PORT_NSP_CONNECTED;
    } else if(m->flags==PORT_NSP_DI &&
             ((l->state!=PORT_NSP_CONNECTED&&l->state!=PORT_NSP_DISCONNECTING)||
              m->source_link==l->remote_link)) {
        l->last_reason=m->reason;
        l->state=l->state==PORT_NSP_CONNECTED?
            PORT_NSP_CLOSED:PORT_NSP_REFUSED;
    } else if(m->flags==PORT_NSP_DC && l->state==PORT_NSP_DISCONNECTING &&
              m->source_link==l->remote_link) {
        l->last_reason=m->reason;
        l->state=PORT_NSP_CLOSED;
    } else {
        ++l->rejected;return PORT_NSP_ENOTSUP;
    }
    ++l->received;return PORT_NSP_OK;
}
int port_nsp_link_tick(struct port_nsp_link *l,uint32_t now) {
    if(!l)return PORT_NSP_EINVAL;
    if(l->state!=PORT_NSP_CI_SENT&&l->state!=PORT_NSP_ACKED&&
       l->state!=PORT_NSP_DISCONNECTING)return 0;
    if((uint32_t)(now-l->last_tx_tick)<PORT_NSP_PROBE_RETRY_TICKS)return 0;
    l->last_tx_tick=now;
    if(l->state==PORT_NSP_DISCONNECTING) {
        l->state=PORT_NSP_TIMED_OUT;++l->timeouts;return -1;
    }
    if(l->attempts>=PORT_NSP_PROBE_MAX_RETRIES) {
        l->state=PORT_NSP_TIMED_OUT;++l->timeouts;return -1;
    }
    ++l->attempts;return 1;
}
int port_nsp_link_disconnect(struct port_nsp_link *l,uint32_t now) {
    if(!l||l->state!=PORT_NSP_CONNECTED||!l->remote_link)
        return PORT_NSP_EINVAL;
    l->state=PORT_NSP_DISCONNECTING;l->last_tx_tick=now;
    return PORT_NSP_OK;
}
