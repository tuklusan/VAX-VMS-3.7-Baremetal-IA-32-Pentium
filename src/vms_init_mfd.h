#ifndef VMS_INIT_MFD_H
#define VMS_INIT_MFD_H
#include <stddef.h>
#include <stdint.h>

/* INIT/SRC/INIMFD.B32: initial ODS-1 and ODS-2 MFD contents. */
typedef uint32_t (*port_init_write_block)(void *context, uint32_t lbn,
                                           const uint8_t block[512]);

enum { PORT_INIT_MFD_NORMAL = 1u, PORT_INIT_MFD_BAD_ARGUMENT = 0u,
       PORT_INIT_MFD_WRITE_FAILED = 2u };

uint32_t port_init_mfd(uint32_t mfd_lbn, unsigned structure1,
                       uint8_t buffer[512], port_init_write_block write_block,
                       void *context);
#endif
