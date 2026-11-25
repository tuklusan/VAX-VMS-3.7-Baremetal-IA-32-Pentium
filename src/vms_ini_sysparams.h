#ifndef VMS_INI_SYSPARAMS_H
#define VMS_INI_SYSPARAMS_H
#include <stdint.h>

#define PORT_VMS37_DEFAULT_MAXPROCESSCNT 72u
#define PORT_VMS37_DEFAULT_IRPCOUNT 60u
#define PORT_VMS37_DEFAULT_LRPCOUNT 4u
#define PORT_VMS37_DEFAULT_LRPSIZE 576u
#define PORT_VMS37_DEFAULT_SRPCOUNT 120u
#define PORT_VMS37_DEFAULT_SRPSIZE 96u
#define PORT_VMS37_DEFAULT_NPAGEDYN 64000u
#define PORT_VMS37_DEFAULT_MPW_WRTCLUSTER 96u
#define PORT_VMS37_DEFAULT_LOCKIDTBL 128u
#define PORT_VMS37_DEFAULT_RESHASHTBL 32u
#define PORT_VMS37_DEFAULT_DEADLOCK_WAIT 10u
#define PORT_VMS37_DEFAULT_BALSETCNT 36u
#define PORT_VMS37_DEFAULT_WSMAX 1024u

/* Native subset of SYSPARAM.MAR defaults consumed by INIT. SYSBOOT's
 * PARAMS.DAT override path is not yet ported; callers must not label these
 * defaults as the configured values when a parameter file is present. */
struct port_vms37_init_params {
    uint32_t max_processes;
    uint32_t realtime_spts;
    uint32_t irp_count, lrp_count, lrp_size, srp_count, srp_size;
    uint32_t nonpaged_dynamic_bytes;
    uint32_t modified_page_fault_cluster;
    uint32_t lock_id_slots;
    uint32_t resource_hash_slots;
    uint32_t deadlock_wait_seconds;
    uint32_t balance_set_count;
    uint32_t max_working_set;
};

enum { PORT_VMS37_SYSPARAM_OK=1u, PORT_VMS37_SYSPARAM_BAD=2u };
uint32_t port_vms37_init_param_defaults(struct port_vms37_init_params *);
#endif
