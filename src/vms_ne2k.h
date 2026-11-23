#ifndef PORTLAB_VMS_NE2K_H
#define PORTLAB_VMS_NE2K_H
#include <stdint.h>
/* ISA NE2000 / DP8390: polled remote DMA with a 16-bit data port. */
#define PORT_NE2K_FRAME_MAX 1518u
#define PORT_NE2K_FRAME_MIN 60u
struct port_ne2k_bus {
    uint8_t (*in8)(void *ctx,uint16_t port);
    uint16_t (*in16)(void *ctx,uint16_t port);
    void (*out8)(void *ctx,uint16_t port,uint8_t value);
    void *ctx;
    void (*out16)(void *ctx,uint16_t port,uint16_t value);
};
struct port_ne2k {
    uint16_t base;
    uint8_t mac[6];
    uint8_t rx_start,rx_stop,tx_start;
    uint32_t reset_count,rx_packets,tx_packets,rx_drops,tx_errors;
};
enum { PORT_NE2K_OK=0, PORT_NE2K_INVALID=-1, PORT_NE2K_MISSING=-2,
       PORT_NE2K_TIMEOUT=-3, PORT_NE2K_BAD_MAC=-4, PORT_NE2K_BUSY=-5,
       PORT_NE2K_BUFFER=-6, PORT_NE2K_CORRUPT=-7 };
int port_ne2k_probe(struct port_ne2k *nic,const struct port_ne2k_bus *bus,uint16_t base);
/* Update PAR station address; useful when taking a DECnet node MAC. */
int port_ne2k_set_station(struct port_ne2k *nic,const struct port_ne2k_bus *bus,
                          const uint8_t mac[6]);
int port_ne2k_pending(struct port_ne2k *nic,const struct port_ne2k_bus *bus);
/* len is a complete Ethernet frame, without FCS. Output always padded to >=60. */
int port_ne2k_send(struct port_ne2k *nic,const struct port_ne2k_bus *bus,
                   const uint8_t *frame,unsigned len);
/* Single polling receive; returns frame length or 0 if ring empty. */
int port_ne2k_recv(struct port_ne2k *nic,const struct port_ne2k_bus *bus,
                   uint8_t *frame,unsigned capacity);
#endif
