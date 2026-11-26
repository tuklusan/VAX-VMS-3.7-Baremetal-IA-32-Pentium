#ifndef VMS_SYSINIT_NAMES_H
#define VMS_SYSINIT_NAMES_H

#include <stddef.h>
#include <stdint.h>
#include "vms_log_system.h"

/* SYSINI/SRC/SYSINIT.MAR system-table entries in SIP_A_CRELOG. */
#define PORT_SIP_NAME_COUNT 3u
#define PORT_SIP_NAME_MAX 32u

enum { PORT_SIP_OK=1, PORT_SIP_BADARG=0, PORT_SIP_TOOLONG=2,
       PORT_SIP_SERVICE_ERROR=4 };

struct port_sip_logical_ops {
    uint32_t (*translate)(void *,const char *name,char *buffer,
                          unsigned capacity,unsigned *received);
    uint32_t (*create_system)(void *,const char *name,const char *value);
};

uint32_t port_sip_create_system_names(const struct port_sip_logical_ops *ops,
                                       void *context,
                                       const uint8_t top_system[10]);

/* Compose SYSINIT's three $CRELOG operations with the bounded resident
 * SYSTEM RSHT. Record storage and executive serialization belong to caller. */
struct port_sip_log_table_context {
    struct port_log_system_table *table;
    uint8_t (*records)[PORT_LOG_MAX_REC_LEN];
    const uint32_t *record_addresses;
    unsigned created;
};
uint32_t port_sip_create_system_names_in_table(
    struct port_log_system_table *,uint8_t [PORT_SIP_NAME_COUNT][PORT_LOG_MAX_REC_LEN],
    const uint32_t [PORT_SIP_NAME_COUNT],const uint8_t top_system[10]);
#endif
