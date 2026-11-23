#ifndef PORTLAB_MIRROR_SERVER_H
#define PORTLAB_MIRROR_SERVER_H
#include <stdint.h>
#include "vms_nsp.h"
#include "vms_nsp_reliable.h"
#include "vms_nsp_data.h"
/* One passive Phase IV Mirror object-25 link; diagnostic only. */
#define PORT_MIRROR_SERVER_LINK 0x5757u
#define PORT_MIRROR_SERVER_BUSY (-2)
#define PORT_MIRROR_SERVER_ORDER (-3)
struct port_mirror_server {
    uint16_t local_node, peer_node, local_link, peer_link;
    uint8_t active;
    struct port_nsp_reliable transport;
    uint8_t replay[11u+PORT_NSP_DATA_MAX_BYTES];
    unsigned replay_len;
    uint32_t accepts, dup_connects, rejects, echoed, bad_data;
};
void port_mirror_server_init(struct port_mirror_server *s);
/* 1=accept new, 2=duplicate request (re-send CA/CC), negative=reject */
int port_mirror_server_connect(struct port_mirror_server *s,
                 uint16_t local_node,uint16_t peer_node,
                 const struct port_nsp_message *ci);
int port_mirror_server_matches(const struct port_mirror_server *s,
                               uint16_t peer_node,uint16_t dst,uint16_t src);
/* positive=encoded outgoing NSP DATA size, 0=duplicate (ACK only),
 * -3=wrong sequence (ACK last good), other negatives=no ACK */
int port_mirror_server_data(struct port_mirror_server *s,uint16_t peer_node,
                            const uint8_t *segment,unsigned n,uint32_t now);
int port_mirror_server_ack(struct port_mirror_server *s,uint16_t peer_node,
                           const uint8_t *segment,unsigned n);
int port_mirror_server_tick(struct port_mirror_server *s,uint32_t now);
void port_mirror_server_close(struct port_mirror_server *s);
#endif
