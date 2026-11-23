#ifndef PORTLAB_SESSION_H
#define PORTLAB_SESSION_H
#include <stdint.h>
/* Small, independent Phase IV Session Control CONNECT request fixture.
 * Contains a numbered destination object, empty source name, no credentials,
 * and no optional data. No authentication, proxy or application services. */
#define PORT_SESSION_REQUEST_BYTES 14u
#define PORT_SESSION_MENUVER 0x03u
#define PORT_SESSION_MIRROR_OBJECT 25u
#define PORT_SESSION_OK 0
#define PORT_SESSION_EFORMAT (-1)
int port_session_build_request(uint8_t *buf,unsigned cap,uint8_t dest_object);
int port_session_parse_request(const uint8_t *buf,unsigned len,
                               uint8_t *dest_object);
#endif
