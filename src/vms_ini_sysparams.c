#include "vms_ini_sysparams.h"


/* Values are the original VMS 3.7 SYSPARAM.MAR defaults, not port-tuned
 * capacities. The matching parameter declarations are recorded in
 * source-coverage.json. */
uint32_t port_vms37_init_param_defaults(struct port_vms37_init_params *p){
    if(!p)return PORT_VMS37_SYSPARAM_BAD;
    *p=(struct port_vms37_init_params){
        .max_processes=PORT_VMS37_DEFAULT_MAXPROCESSCNT, /* MAXPROCESSCNT */
        .realtime_spts=0u,              /* REALTIME_SPTS */
        .irp_count=PORT_VMS37_DEFAULT_IRPCOUNT, /* IRPCOUNT */
        .lrp_count=PORT_VMS37_DEFAULT_LRPCOUNT, /* LRPCOUNT */
        .lrp_size=PORT_VMS37_DEFAULT_LRPSIZE,   /* LRPSIZE */
        .srp_count=PORT_VMS37_DEFAULT_SRPCOUNT, /* SRPCOUNT */
        .srp_size=PORT_VMS37_DEFAULT_SRPSIZE,   /* SRPSIZE */
        .nonpaged_dynamic_bytes=PORT_VMS37_DEFAULT_NPAGEDYN, /* NPAGEDYN */
        .paged_dynamic_bytes=PORT_VMS37_DEFAULT_PAGEDYN, /* PAGEDYN */
        .modified_page_fault_cluster=PORT_VMS37_DEFAULT_MPW_WRTCLUSTER,
        .lock_id_slots=PORT_VMS37_DEFAULT_LOCKIDTBL, /* LOCKIDTBL */
        .resource_hash_slots=PORT_VMS37_DEFAULT_RESHASHTBL, /* RESHASHTBL */
        .deadlock_wait_seconds=PORT_VMS37_DEFAULT_DEADLOCK_WAIT,
        .balance_set_count=PORT_VMS37_DEFAULT_BALSETCNT, /* BALSETCNT */
        .max_working_set=PORT_VMS37_DEFAULT_WSMAX, /* WSMAX */
        .system_log_hash_entries=PORT_VMS37_DEFAULT_LOGSHASHTBL,
        .group_log_hash_entries=PORT_VMS37_DEFAULT_LOGGHASHTBL,
        .process_log_hash_entries=PORT_VMS37_DEFAULT_LOGPHASHTBL,
        .terminal_prompt_wait=-1 /* TIMEPROMPTWAIT: negative means wait forever */
    };
    return PORT_VMS37_SYSPARAM_OK;
}
