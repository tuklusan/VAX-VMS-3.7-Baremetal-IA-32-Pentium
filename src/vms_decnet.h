#ifndef PORTLAB_DECNET_H
#define PORTLAB_DECNET_H
#include <stdint.h>
/* DECnet Phase IV Ethernet-II framing and short routing header only.
 * No network-layer forwarding, NSP, NCP, or DECnet connection services. */
#define PORT_DECNET_ETHER_TYPE 0x6003u
#define PORT_DECNET_SHORT_FLAGS 0x02u
#define PORT_DECNET_DATA_MAX 1498u
#define PORT_DECNET_HEADER_BYTES 22u
#define PORT_DECNET_HELLO_FRAME_BYTES 50u
#define PORT_DECNET_HELLO_INTERVAL_TICKS 1500u /* PIT programmed to ~100 Hz */
#define PORT_DECNET_MAX_ROUTERS 8u
struct port_decnet_router_hello {
    uint16_t nodeid,blocksize,timer;
    uint8_t priority,info,list_count,mac[6];
};
struct port_decnet_router_entry {
    struct port_decnet_router_hello hello;
    uint32_t last_seen_tick;
    uint8_t valid;
};
struct port_decnet_adjacencies {
    struct port_decnet_router_entry routers[PORT_DECNET_MAX_ROUTERS];
    uint32_t learned,expired,overflow;
};
struct port_decnet_hello {
    uint16_t nodeid,blocksize,timer;
    uint8_t neighbor_mac[6];
};
struct port_decnet_short {
    uint16_t src_node,dst_node;
    uint8_t flags,visits;
    const uint8_t *payload;
    unsigned payload_len;
};
enum { PORT_DECNET_OK=0, PORT_DECNET_INVALID=-1,
       PORT_DECNET_FORMAT=-2, PORT_DECNET_UNSUPPORTED=-3 };
int port_decnet_node(unsigned area,unsigned number,uint16_t *nodeid);
void port_decnet_node_mac(uint16_t nodeid,uint8_t mac[6]);
int port_decnet_build_short(uint8_t *frame,unsigned cap,uint16_t src,
                            uint16_t dst,const uint8_t *payload,unsigned len);
int port_decnet_build_endnode_hello_to(uint8_t *frame,unsigned cap,
                                      uint16_t local,uint16_t blocksize,
                                      uint16_t hello_period,const uint8_t router_mac[6]);
int port_decnet_build_endnode_hello(uint8_t *frame,unsigned cap,
                                      uint16_t local,uint16_t blocksize,
                                      uint16_t hello_period);
int port_decnet_parse_router_hello(const uint8_t *frame,unsigned len,
                                    struct port_decnet_router_hello *hello);
int port_decnet_adj_update(struct port_decnet_adjacencies *adj,
                           const struct port_decnet_router_hello *router,
                           uint32_t now_ticks);
void port_decnet_adj_expire(struct port_decnet_adjacencies *adj,uint32_t now_ticks);
const struct port_decnet_router_entry *port_decnet_adj_best(
        const struct port_decnet_adjacencies *adj,uint16_t local_nodeid);
int port_decnet_parse_endnode_hello(const uint8_t *frame,unsigned len,
                                      struct port_decnet_hello *hello);
int port_decnet_parse_short(const uint8_t *frame,unsigned len,
                            struct port_decnet_short *packet);
#endif
