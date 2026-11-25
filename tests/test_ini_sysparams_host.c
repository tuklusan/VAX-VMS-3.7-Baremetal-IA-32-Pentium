#include "vms_ini_sysparams.h"
#include <stdio.h>

int main(void){
    struct port_vms37_init_params p={0};
    if(port_vms37_init_param_defaults(&p)!=PORT_VMS37_SYSPARAM_OK||
       p.max_processes!=72u||p.realtime_spts!=0u||p.irp_count!=60u||
       p.lrp_count!=4u||p.lrp_size!=576u||p.srp_count!=120u||
       p.srp_size!=96u||p.nonpaged_dynamic_bytes!=64000u||
       p.lock_id_slots!=128u||
       p.resource_hash_slots!=32u||p.deadlock_wait_seconds!=10u||
       p.balance_set_count!=36u||p.max_working_set!=1024u||
       port_vms37_init_param_defaults(0)!=PORT_VMS37_SYSPARAM_BAD){
        fputs("VMS 3.7 SYSPARAM defaults failed\n",stderr);
        return 1;
    }
    puts("VMS 3.7 INIT SYSPARAM defaults match original parameter declarations");
    return 0;
}
