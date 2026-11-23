#ifndef VMS37_SYSQIO_ARGS_H
#define VMS37_SYSQIO_ARGS_H
#include <stddef.h>
#include <stdint.h>
#include "vms_dq_fdt.h"

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

enum { PORT_V37_SYSQIO_PRECHECK_OK=1u,
       PORT_V37_SYSQIO_PRECHECK_BAD=2u,
       PORT_V37_SYSQIO_PRECHECK_NOPRIV=4u,
       PORT_V37_SYSQIO_PRECHECK_ILLFUNC=6u,
       PORT_V37_SYSQIO_PRECHECK_OFFLINE=8u };
struct port_v37_sysqio_device {
    const struct port_dq_fdt *fdt;
    size_t fdt_bytes;
    uint8_t ccb_access_limit; /* CCB$B_AMOD stores MAXACMODE + 1. */
    uint8_t online;
};
struct port_v37_sysqio_dispatch {
    uint16_t full_function;
    uint8_t function_code,buffered,action_class;
};

/* SYSQIOREQ.MAR channel access, FDT legality and offline-device checks.
 * These result values are internal and are not VMS condition codes. */
uint32_t port_v37_sysqio_preflight(const struct port_v37_sysqio_args *,
    uint8_t caller_mode,const struct port_v37_sysqio_device *,
    struct port_v37_sysqio_dispatch *);
#endif
