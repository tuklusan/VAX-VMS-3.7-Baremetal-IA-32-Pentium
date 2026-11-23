#ifndef VMS37_SYSQIO_ARGS_H
#define VMS37_SYSQIO_ARGS_H
#include <stddef.h>
#include <stdint.h>

/* Native decoder for the fixed SYS$QIO/EXE$QIO argument list. The argument
 * list retains the VAX AP layout: count longword followed by 12 longwords. */
enum { PORT_V37_SYSQIO_REQUIRED_ARGS=12u,
       PORT_V37_SYSQIO_LIST_BYTES=52u,
       PORT_V37_SYSQIO_ARGS_OK=1u,
       PORT_V37_SYSQIO_ARGS_BAD=2u };

struct port_v37_sysqio_args {
    uint32_t event_flag,channel;
    uint16_t function;
    uint16_t reserved;
    uint32_t iosb,ast_address,ast_parameter;
    uint32_t parameter[6];
};

uint32_t port_v37_sysqio_decode_args(const void *argument_list,
    size_t accessible_bytes,struct port_v37_sysqio_args *decoded);
#endif
