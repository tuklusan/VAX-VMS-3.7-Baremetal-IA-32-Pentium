#ifndef PORTLAB_NSP_DATA_H
#define PORTLAB_NSP_DATA_H
#include <stdint.h>
/* Canonical NSP data and acknowledgment segment subset, no flow control. */
#define PORT_NSP_DATA_MOM 0x60u /* first+last, entire record */
#define PORT_NSP_DATA_ACK 0x04u
#define PORT_NSP_DATA_MAX_BYTES 512u
struct port_nsp_data {
    uint8_t flags;
    uint16_t destination_link,source_link;
    uint16_t ack_data,ack_other,sequence;
    const uint8_t *body;
    unsigned body_len;
};
struct port_nsp_data_ack {
    uint16_t destination_link,source_link,ack_data,ack_other;
};
int port_nsp_data_build(uint8_t *buf,unsigned cap,uint8_t flags,
                         uint16_t destination,uint16_t source,
                         uint16_t ack_data,uint16_t ack_other,uint16_t seq,
                         const uint8_t *body,unsigned body_len);
int port_nsp_data_parse(const uint8_t *buf,unsigned n,struct port_nsp_data *out);
int port_nsp_data_ack_build(uint8_t *buf,unsigned cap,
                             uint16_t destination,uint16_t source,
                             uint16_t ack_data,uint16_t ack_other);
int port_nsp_data_ack_parse(const uint8_t *buf,unsigned n,
                             struct port_nsp_data_ack *out);
#endif
