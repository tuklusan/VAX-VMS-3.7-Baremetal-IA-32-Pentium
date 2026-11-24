#ifndef VMS37_SYSASSIGN_H
#define VMS37_SYSASSIGN_H
#include <stddef.h>
#include <stdint.h>
#include "vms_ioc_phys.h"
#include "vms_v37_ccb.h"

/* Local SYS$ASSIGN front end through physical IOC$SEARCHDEV and the source
 * CCB/owner path. Descriptor probing/translation, MAXACMODE calculation,
 * mailbox association, spooled and remote-device paths remain caller work. */
struct port_v37_sysassign_local_request {
    const uint8_t *device_database;
    size_t device_database_bytes;
    uint32_t device_database_base;
    const struct port_v37_device_result *devices;
    const char *device_name;
    size_t device_name_bytes;
    uint8_t *ccb_base;
    size_t ccb_preceding_bytes;
    uint16_t channel_count;
    uint16_t *high_water_channel;
    uint32_t process_pid,arb_va,mailbox_ucb;
    uint8_t effective_max_acmode;
    const struct port_v37_assign_ops *assign_ops;
};
uint32_t port_v37_sysassign_local(
    const struct port_v37_sysassign_local_request *,uint16_t *channel);
#endif
