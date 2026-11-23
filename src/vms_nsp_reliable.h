#ifndef PORTLAB_NSP_RELIABLE_H
#define PORTLAB_NSP_RELIABLE_H
#include <stdint.h>
/* A deliberately bounded Phase-IV NSP stop-and-wait subset. One outstanding
 * DATA segment; 12-bit modulo sequence values. No fragmentation or flow
 * control. All callbacks belong to the caller's safe-point, never IRQ0. */
#define PORT_NSP_RETRY_TICKS 200u
#define PORT_NSP_RETRY_LIMIT 3u
#define PORT_NSP_RX_INVALID (-1)
#define PORT_NSP_RX_OUT_OF_ORDER 0
#define PORT_NSP_RX_NEW 1
#define PORT_NSP_RX_DUPLICATE 2
struct port_nsp_reliable {
    uint16_t next_rx, next_tx, last_rx, pending_seq;
    uint32_t pending_tick;
    uint8_t have_rx, pending, attempts;
    uint32_t accepted, duplicates, out_of_order;
    uint32_t acked, rejected_ack, retries, timeouts;
};
void port_nsp_reliable_init(struct port_nsp_reliable *t);
int port_nsp_rx_sequence(struct port_nsp_reliable *t, uint16_t seq);
int port_nsp_tx_begin(struct port_nsp_reliable *t, uint32_t tick,
                      uint16_t *sequence);
/* 1=outstanding sequence acknowledged, 0=stale/unexpected */
int port_nsp_tx_ack(struct port_nsp_reliable *t, uint16_t sequence);
/* 1=retransmit existing DATA bytes; -1=gave up; 0=no work */
int port_nsp_tx_tick(struct port_nsp_reliable *t, uint32_t tick);
void port_nsp_tx_cancel(struct port_nsp_reliable *t);
#endif
