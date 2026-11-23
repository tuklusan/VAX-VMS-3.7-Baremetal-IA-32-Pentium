#ifndef PORTLAB_NSP_H
#define PORTLAB_NSP_H
#include <stdint.h>
/* Bounded wire-format subset of Phase IV Network Services Protocol.
 * This is not DEC's NSP implementation and cannot run DECnet applications. */
#define PORT_NSP_CI     0x18u
#define PORT_NSP_RCI    0x68u
#define PORT_NSP_CA     0x24u
#define PORT_NSP_CC     0x28u
#define PORT_NSP_DI     0x38u
#define PORT_NSP_DC     0x48u
#define PORT_NSP_MAX_SEGMENT 576u
#define PORT_NSP_NO_FLOW_CONTROL 0x00u
#define PORT_NSP_OK 0
#define PORT_NSP_EINVAL (-1)
#define PORT_NSP_EFORMAT (-2)
#define PORT_NSP_ENOTSUP (-3)
struct port_nsp_message {
    uint8_t flags, services, info;
    uint16_t destination_link, source_link, segment_size, reason;
    const uint8_t *optional;
    unsigned optional_length;
};
/* Parse and build only connection-control messages. DATA/ACK data frames
 * remain unsupported. The optional portion is an opaque bounded view. */
int port_nsp_parse(const uint8_t *bytes,unsigned length,
                   struct port_nsp_message *msg);
int port_nsp_build_connect(uint8_t *buf,unsigned capacity,uint8_t flags,
                           uint16_t source_link,uint16_t destination_link,
                           uint16_t segment_size,
                           const uint8_t *session,unsigned session_len);
int port_nsp_build_conn_ack(uint8_t *buf,unsigned capacity,uint16_t destination_link);
int port_nsp_build_disconnect(uint8_t *buf,unsigned capacity,uint8_t flags,
                              uint16_t destination_link,uint16_t source_link,
                              uint16_t reason);
#endif
