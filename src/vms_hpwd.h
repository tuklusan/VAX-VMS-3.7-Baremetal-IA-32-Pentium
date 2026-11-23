#ifndef VMS_HPWD_H
#define VMS_HPWD_H
#include <stdint.h>

/* Native implementation of the LOGIN HPWD service for byte descriptors. */
#define PORT_HPWD_CRC 0u
#define PORT_HPWD_PURDY 1u

/* The input lengths are VAX descriptor word lengths. The result is an
 * eight-byte little-endian VMS password field. Returns one on success. */
int port_hpwd(uint8_t hash[8], const uint8_t *password, uint16_t password_len,
              uint8_t algorithm, uint16_t salt,
              const uint8_t *username, uint16_t username_len);
int port_hpwd_matches(const uint8_t stored[8], const uint8_t *password,
                      uint16_t password_len, uint8_t algorithm, uint16_t salt,
                      const uint8_t *username, uint16_t username_len);
uint32_t port_hpwd_selftest(void);
#endif
