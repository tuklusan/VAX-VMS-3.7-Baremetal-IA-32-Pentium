#include "vms_ini_sysparams.h"

/* Values are the original VMS 3.7 SYSPARAM.MAR defaults, not port-tuned
 * capacities. The matching parameter declarations are recorded in
 * source-coverage.json. */
uint32_t port_vms37_init_param_defaults(struct port_vms37_init_params *p){
    if(!p)return PORT_VMS37_SYSPARAM_BAD;
    *p=(struct port_vms37_init_params){
        .max_processes=72u,             /* MAXPROCESSCNT */
        .realtime_spts=0u,              /* REALTIME_SPTS */
        .lock_id_slots=128u,            /* LOCKIDTBL */
        .resource_hash_slots=32u,       /* RESHASHTBL */
        .deadlock_wait_seconds=10u,     /* DEADLOCK_WAIT */
        .balance_set_count=36u,         /* BALSETCNT */
        .max_working_set=1024u          /* WSMAX */
    };
    return PORT_VMS37_SYSPARAM_OK;
}
