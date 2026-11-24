#ifndef VMS37_SYSASSIGN_H
#define VMS37_SYSASSIGN_H
#include <stddef.h>
#include <stdint.h>
#include "vms_ioc_phys.h"
#include "vms_v37_ccb.h"
#include "vms_sysqio_args.h"
#include "vms_exe_access.h"

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
struct port_v37_sysassign_descriptor_request {
    const struct port_v37_sysassign_local_request *local;
    const struct port_exe_access *memory;
    uint32_t device_name_descriptor;
    uint8_t caller_access_mode,requested_access_mode;
};
/* SYSASSIGN.MAR entry: clear channel, read the source string descriptor,
 * maximize the requested mode, then enter the native local assignment path. */
uint32_t port_v37_sysassign_descriptor(
    const struct port_v37_sysassign_descriptor_request *,uint16_t *channel);
uint32_t port_v37_sysassign_local(
    const struct port_v37_sysassign_local_request *,uint16_t *channel);

/* Bind a process's assigned source CCB table to SYSQIOREQ channel lookup.
 * The device description is supplied for the selected UCB by the caller. */
struct port_v37_sysassign_qio_context {
    uint8_t *ccb_base;
    size_t ccb_preceding_bytes;
    uint16_t maximum_channel_offset;
    uint32_t owner_pid;
    const uint8_t *device_database;
    size_t device_database_bytes;
    uint32_t device_database_base,assigned_ucb;
    const struct port_v37_sysqio_device *device;
};
int port_v37_sysassign_lookup_qio_channel(void *,uint32_t,uint32_t,
    struct port_v37_sysqio_resolved_channel *);
/* SYSQIOREQ.MAR SUCCES charges CCB$W_IOC before invoking FDT actions. */
uint32_t port_v37_sysassign_qio_charge(void *,uint32_t,uint32_t,
    uint8_t,const struct port_v37_assign_ops *,uint16_t *);
#endif
