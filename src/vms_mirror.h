#ifndef PORTLAB_DECNET_MIRROR_H
#define PORTLAB_DECNET_MIRROR_H
#include <stdint.h>
/* DECnet network management loopback, object 25:
 * request 0x00 + opaque data; success reply 0x01 + unchanged data;
 * malformed/unsupported function reply 0xff. No authentication/session ABI. */
#define PORT_MIRROR_CALL 0x00u
#define PORT_MIRROR_OK 0x01u
#define PORT_MIRROR_ERROR 0xffu
#define PORT_MIRROR_MAX 512u
int port_mirror_make_request(uint8_t *out,unsigned cap,
                             const uint8_t *data,unsigned len);
int port_mirror_reply(uint8_t *out,unsigned cap,
                      const uint8_t *request,unsigned len);
/* 0=success matching original request, -1=wrong/malformed, -2=error reply */
int port_mirror_verify(const uint8_t *request,unsigned request_len,
                       const uint8_t *response,unsigned response_len);
#endif
