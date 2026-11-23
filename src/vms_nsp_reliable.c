#include "vms_nsp_reliable.h"
void port_nsp_reliable_init(struct port_nsp_reliable *t) {
    if(!t)return;
    *t=(struct port_nsp_reliable){0};
    t->next_rx=1;
    t->next_tx=1;
}
int port_nsp_rx_sequence(struct port_nsp_reliable *t,uint16_t seq) {
    if(!t||seq>0x0fffu)return PORT_NSP_RX_INVALID;
    if(seq==t->next_rx) {
        t->last_rx=seq;t->have_rx=1;
        t->next_rx=(uint16_t)((seq+1u)&0x0fffu);
        ++t->accepted;return PORT_NSP_RX_NEW;
    }
    if(t->have_rx&&seq==t->last_rx) {
        ++t->duplicates;return PORT_NSP_RX_DUPLICATE;
    }
    ++t->out_of_order;return PORT_NSP_RX_OUT_OF_ORDER;
}
int port_nsp_tx_begin(struct port_nsp_reliable *t,uint32_t now,
                      uint16_t *sequence) {
    if(!t||!sequence||t->pending)return -1;
    t->pending=1;
    t->pending_seq=t->next_tx;
    t->next_tx=(uint16_t)((t->next_tx+1u)&0x0fffu);
    t->pending_tick=now;t->attempts=1;
    *sequence=t->pending_seq;return 0;
}
int port_nsp_tx_ack(struct port_nsp_reliable *t,uint16_t sequence) {
    if(!t||sequence>0x0fffu)return -1;
    if(t->pending&&sequence==t->pending_seq) {
        t->pending=0;t->attempts=0;++t->acked;return 1;
    }
    ++t->rejected_ack;return 0;
}
int port_nsp_tx_tick(struct port_nsp_reliable *t,uint32_t now) {
    if(!t||!t->pending)return 0;
    if((uint32_t)(now-t->pending_tick)<PORT_NSP_RETRY_TICKS)return 0;
    t->pending_tick=now;
    if(t->attempts>=PORT_NSP_RETRY_LIMIT) {
        t->pending=0;t->attempts=0;++t->timeouts;return -1;
    }
    ++t->attempts;++t->retries;return 1;
}
void port_nsp_tx_cancel(struct port_nsp_reliable *t) {
    if(!t)return;
    t->pending=0;t->attempts=0;
}
