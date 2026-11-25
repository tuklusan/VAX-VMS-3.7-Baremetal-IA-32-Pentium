#ifndef VMS_INI_SYSPARAMS_H
#define VMS_INI_SYSPARAMS_H
#include <stdint.h>

/* Native subset of SYSPARAM.MAR defaults consumed by INIT. SYSBOOT's
 * PARAMS.DAT override path is not yet ported; callers must not label these
 * defaults as the configured values when a parameter file is present. */
struct port_vms37_init_params {
    uint32_t max_processes;
    uint32_t realtime_spts;
    uint32_t irp_count, lrp_count, lrp_size, srp_count, srp_size;
    uint32_t nonpaged_dynamic_bytes;
    uint32_t lock_id_slots;
    uint32_t resource_hash_slots;
    uint32_t deadlock_wait_seconds;
    uint32_t balance_set_count;
    uint32_t max_working_set;
};

enum { PORT_VMS37_SYSPARAM_OK=1u, PORT_VMS37_SYSPARAM_BAD=2u };
uint32_t port_vms37_init_param_defaults(struct port_vms37_init_params *);
#endif
