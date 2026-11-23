#ifndef PORTLAB_NSP_LINK_H
#define PORTLAB_NSP_LINK_H
#include <stdint.h>
#include "vms_nsp.h"
/* One outbound diagnostic NSP logical-link probe. No Session Control. */
enum port_nsp_state {
    PORT_NSP_CLOSED=0,PORT_NSP_CI_SENT=1,PORT_NSP_ACKED=2,
    PORT_NSP_CONNECTED=3,PORT_NSP_DISCONNECTING=4,
    PORT_NSP_REFUSED=5,PORT_NSP_TIMED_OUT=6
};
#define PORT_NSP_PROBE_RETRY_TICKS 300u
#define PORT_NSP_PROBE_MAX_RETRIES 3u
struct port_nsp_link {
    uint16_t local_node,remote_node,local_link,remote_link;
    uint16_t last_reason;
    enum port_nsp_state state;
    uint8_t attempts;
    uint32_t last_tx_tick;
    uint32_t received, rejected, timeouts;
};
int port_nsp_link_open(struct port_nsp_link *link,uint16_t local,
                       uint16_t remote,uint16_t link_id,uint32_t now);
int port_nsp_link_rx(struct port_nsp_link *link,uint16_t src_node,
                     uint16_t dst_node,const struct port_nsp_message *msg);
/* Tick returns 1 when retransmission needed, -1 on timeout, 0 otherwise.
 * Only call after the corresponding send is actually attempted. */
int port_nsp_link_tick(struct port_nsp_link *link,uint32_t now);
int port_nsp_link_disconnect(struct port_nsp_link *link,uint32_t now);
void port_nsp_link_reset(struct port_nsp_link *link);
#endif
