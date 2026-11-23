#include "vms_decnet.h"
static uint16_t getle(const uint8_t *p) {
    return (uint16_t)((unsigned)p[0]|((unsigned)p[1]<<8));
}
static void putle(uint8_t *p,uint16_t value) {
    p[0]=(uint8_t)value;p[1]=(uint8_t)(value>>8);
}
int port_decnet_node(unsigned area,unsigned number,uint16_t *nodeid) {
    if(!nodeid||area<1||area>63||number<1||number>1023)
        return PORT_DECNET_INVALID;
    *nodeid=(uint16_t)((area<<10)|number);
    return PORT_DECNET_OK;
}
void port_decnet_node_mac(uint16_t id,uint8_t mac[6]) {
    mac[0]=0xaa;mac[1]=0;mac[2]=4;mac[3]=0;
    mac[4]=(uint8_t)id;mac[5]=(uint8_t)(id>>8);
}
int port_decnet_build_short(uint8_t *frame,unsigned cap,uint16_t src,
                            uint16_t dst,const uint8_t *payload,unsigned len) {
    if(!frame||(!payload&&len)||!src||!dst||
       (src&0x03ffu)==0||(dst&0x03ffu)==0||
       len>PORT_DECNET_DATA_MAX-6u||
       cap<PORT_DECNET_HEADER_BYTES+len)
        return PORT_DECNET_INVALID;
    port_decnet_node_mac(dst,frame);
    port_decnet_node_mac(src,frame+6);
    frame[12]=0x60;frame[13]=0x03;
    /* little-endian routing-layer length; excludes these 2 bytes */
    putle(frame+14,(uint16_t)(6+len));
    frame[16]=PORT_DECNET_SHORT_FLAGS;
    putle(frame+17,dst);
    putle(frame+19,src);
    frame[21]=0; /* visits */
    for(unsigned i=0;i<len;i++) frame[22+i]=payload[i];
    return (int)(22+len);
}
int port_decnet_parse_short(const uint8_t *frame,unsigned len,
                            struct port_decnet_short *p) {
    if(!frame||!p) return PORT_DECNET_INVALID;
    if(len<PORT_DECNET_HEADER_BYTES||frame[12]!=0x60||frame[13]!=0x03)
        return PORT_DECNET_FORMAT;
    unsigned wire_len=getle(frame+14);
    /* The Ethernet minimum frame length can require extra bytes of padding.
       DECnet's count limits the meaningful payload. */
    if(wire_len<6||wire_len>PORT_DECNET_DATA_MAX||wire_len+16u>len)
        return PORT_DECNET_FORMAT;
    uint8_t flags=frame[16];
    if((flags&0x07u)!=PORT_DECNET_SHORT_FLAGS)
        return PORT_DECNET_UNSUPPORTED; /* Hello/long/padded flags */
    uint16_t dst=getle(frame+17),src=getle(frame+19);
    if(!dst||!src||(src&0x3ffu)==0||(dst&0x3ffu)==0)
        return PORT_DECNET_FORMAT;
    /* Source/destination Ethernet MACs are link-local next hops. A router
       may forward a short packet while retaining its original node IDs. */
    p->src_node=src;p->dst_node=dst;p->flags=flags;p->visits=frame[21];
    p->payload=frame+22;p->payload_len=wire_len-6;
    return PORT_DECNET_OK;
}

/* Ethernet Endnode Hello message. The fixed 34-byte control payload follows
 * the two-byte little-endian message length; Ethernet padding is separate. */
int port_decnet_build_endnode_hello_to(uint8_t *frame,unsigned cap,
                                       uint16_t local,uint16_t blocksize,
                                       uint16_t timer,const uint8_t router_mac[6]) {
    if(!frame||cap<PORT_DECNET_HELLO_FRAME_BYTES||
       !local||(local&0x3ffu)==0||!blocksize||!timer)
        return PORT_DECNET_INVALID;
    static const uint8_t multicast[6]={0xab,0,0,3,0,0};
    for(unsigned i=0;i<6;i++)frame[i]=multicast[i];
    port_decnet_node_mac(local,frame+6);
    frame[12]=0x60;frame[13]=0x03;
    putle(frame+14,34);
    uint8_t *p=frame+16;
    p[0]=0x0d; /* endnode Hello control message */
    p[1]=2;p[2]=0;p[3]=0; /* Phase IV ECO version */
    port_decnet_node_mac(local,p+4);
    p[10]=3; /* Endnode */
    putle(p+11,blocksize);
    p[13]=0; /* area: reserved */
    for(unsigned i=14;i<22;i++)p[i]=0; /* verification seed */
    if(router_mac) {
        for(unsigned i=0;i<6;i++)p[22+i]=router_mac[i];
    } else { /* unknown designated router */
        p[22]=0xaa;p[23]=0;p[24]=4;p[25]=0;p[26]=0;p[27]=0;
    }
    putle(p+28,timer);
    p[30]=0; /* reserved */
    p[31]=2;
    p[32]=0xaa;p[33]=0xaa;
    return (int)PORT_DECNET_HELLO_FRAME_BYTES;
}
int port_decnet_build_endnode_hello(uint8_t *frame,unsigned cap,
                                    uint16_t local,uint16_t blocksize,
                                    uint16_t timer) {
    return port_decnet_build_endnode_hello_to(frame,cap,local,blocksize,timer,0);
}
int port_decnet_parse_endnode_hello(const uint8_t *frame,unsigned len,
                                    struct port_decnet_hello *hello) {
    static const uint8_t multicast[6]={0xab,0,0,3,0,0};
    if(!frame||!hello) return PORT_DECNET_INVALID;
    if(len<PORT_DECNET_HELLO_FRAME_BYTES ||
       frame[12]!=0x60||frame[13]!=3||getle(frame+14)!=34)
        return PORT_DECNET_FORMAT;
    for(unsigned i=0;i<6;i++)if(frame[i]!=multicast[i])return PORT_DECNET_FORMAT;
    const uint8_t *p=frame+16;
    if(p[0]!=0x0d||p[1]!=2||p[2]!=0||p[3]!=0||
       (p[10]&3u)!=3||p[31]!=2||p[32]!=0xaa||p[33]!=0xaa||
       p[13]!=0||p[30]!=0)
        return PORT_DECNET_FORMAT;
    uint16_t node=getle(p+8);
    uint8_t local_mac[6];port_decnet_node_mac(node,local_mac);
    if(!node||(node&0x3ffu)==0)return PORT_DECNET_FORMAT;
    for(unsigned i=0;i<6;i++) {
        if(p[4+i]!=local_mac[i]||frame[6+i]!=local_mac[i])
            return PORT_DECNET_FORMAT;
    }
    unsigned blksize=getle(p+11), timer=getle(p+28);
    if(!blksize||!timer)return PORT_DECNET_FORMAT;
    hello->nodeid=node;hello->blocksize=(uint16_t)blksize;
    hello->timer=(uint16_t)timer;
    for(unsigned i=0;i<6;i++)hello->neighbor_mac[i]=p[22+i];
    return PORT_DECNET_OK;
}

int port_decnet_parse_router_hello(const uint8_t *f,unsigned len,
                                   struct port_decnet_router_hello *h) {
    if(!f||!h)return PORT_DECNET_INVALID;
    if(len<43||f[12]!=0x60||f[13]!=3)return PORT_DECNET_FORMAT;
    unsigned length=getle(f+14);
    if(length<27||length>27+32u*7u||length+16u>len)
        return PORT_DECNET_FORMAT;
    /* Routers address their hello packets to all routers and/or endnodes. */
    if(f[0]!=0xab||f[1]!=0||f[2]!=0||
       (f[3]!=3&&f[3]!=4)||f[4]!=0||f[5]!=0)
        return PORT_DECNET_FORMAT;
    const uint8_t *p=f+16;
    if(p[0]!=0x0b||p[1]!=2||p[2]!=0||p[3]!=0||
       ((p[10]&3u)!=1&& (p[10]&3u)!=2) ||
       p[14]!=0||p[17]!=0 ||p[26]%7u ||
       (unsigned)p[26]+27u!=length ||
       (unsigned)p[18]!=(unsigned)p[26]+8u)
        return PORT_DECNET_FORMAT;
    uint16_t node=getle(p+8);
    uint8_t node_mac[6];port_decnet_node_mac(node,node_mac);
    if(!node||(node&1023u)==0||!getle(p+11)||!getle(p+15))
        return PORT_DECNET_FORMAT;
    for(unsigned i=0;i<6;i++)
        if(p[4+i]!=node_mac[i]||f[6+i]!=node_mac[i])return PORT_DECNET_FORMAT;
    h->nodeid=node;
    h->blocksize=getle(p+11);
    h->priority=p[13];h->info=p[10];h->timer=getle(p+15);
    h->list_count=(uint8_t)(p[26]/7u);
    for(unsigned i=0;i<6;i++)h->mac[i]=node_mac[i];
    return PORT_DECNET_OK;
}
int port_decnet_adj_update(struct port_decnet_adjacencies *adj,
                           const struct port_decnet_router_hello *router,
                           uint32_t now) {
    if(!adj||!router||!router->nodeid||
       (router->nodeid&1023u)==0||!router->timer)
        return PORT_DECNET_INVALID;
    unsigned slot=PORT_DECNET_MAX_ROUTERS;
    for(unsigned i=0;i<PORT_DECNET_MAX_ROUTERS;i++) {
        if(adj->routers[i].valid&&
           adj->routers[i].hello.nodeid==router->nodeid){slot=i;break;}
    }
    if(slot==PORT_DECNET_MAX_ROUTERS) {
        for(unsigned i=0;i<PORT_DECNET_MAX_ROUTERS;i++)
            if(!adj->routers[i].valid){slot=i;break;}
    }
    if(slot==PORT_DECNET_MAX_ROUTERS){++adj->overflow;return PORT_DECNET_INVALID;}
    adj->routers[slot].hello=*router;
    adj->routers[slot].last_seen_tick=now;
    adj->routers[slot].valid=1;
    ++adj->learned;
    return PORT_DECNET_OK;
}
void port_decnet_adj_expire(struct port_decnet_adjacencies *adj,uint32_t now) {
    if(!adj)return;
    for(unsigned i=0;i<PORT_DECNET_MAX_ROUTERS;i++) {
        struct port_decnet_router_entry *entry=&adj->routers[i];
        if(!entry->valid)continue;
        /* Allow three Hello periods; prevent overflow for 16-bit timer. */
        uint32_t timeout=(uint32_t)entry->hello.timer*300u;
        if((uint32_t)(now-entry->last_seen_tick)>=timeout) {
            entry->valid=0;
            ++adj->expired;
        }
    }
}
const struct port_decnet_router_entry *port_decnet_adj_best(
        const struct port_decnet_adjacencies *adj,uint16_t local) {
    if(!adj||!local)return 0;
    const struct port_decnet_router_entry *best=0;
    for(unsigned i=0;i<PORT_DECNET_MAX_ROUTERS;i++) {
        const struct port_decnet_router_entry *e=&adj->routers[i];
        if(!e->valid||((e->hello.nodeid>>10)!=(local>>10)))continue;
        if(!best || e->hello.priority>best->hello.priority ||
           (e->hello.priority==best->hello.priority &&
            e->hello.nodeid>best->hello.nodeid)) best=e;
    }
    return best;
}
