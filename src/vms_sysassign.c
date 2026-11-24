#include "vms_sysassign.h"

uint32_t port_v37_sysassign_local(
    const struct port_v37_sysassign_local_request *r,uint16_t *channel){
    if(!channel)return PORT_V37_CCB_BAD;
    *channel=0u; /* SYSASSIGN clears the caller's channel before lookup. */
    if(!r||!r->device_database||!r->devices||!r->device_name||
       !r->device_name_bytes||!r->ccb_base||!r->high_water_channel||
       !r->channel_count||!r->process_pid||r->effective_max_acmode>3u||
       !r->assign_ops||
       r->device_database_base!=(uint32_t)(uintptr_t)r->device_database)
        return PORT_V37_CCB_BAD;
    struct port_v37_ioc_result found={0};
    const uint32_t search=port_v37_ioc_searchdev(r->device_database,
        r->device_database_bytes,r->device_database_base,r->devices,
        r->device_name,r->device_name_bytes,&found);
    if(search!=PORT_V37_IOC_OK)return search;
    if(found.ucb<r->device_database_base)return PORT_V37_CCB_BAD;
    const uint32_t offset=found.ucb-r->device_database_base;
    if(offset>r->device_database_bytes||
       PORT_V37_UCB_FIXED_BYTES>r->device_database_bytes-offset)
        return PORT_V37_CCB_BAD;
    uint8_t *ucb=(uint8_t *)(uintptr_t)r->device_database+offset;
    uint16_t assigned=0u;
    const uint32_t status=port_v37_ccb_assign_local(r->ccb_base,
        r->ccb_preceding_bytes,r->channel_count,r->high_water_channel,ucb,
        r->device_database_bytes-offset,found.ucb,r->process_pid,r->arb_va,
        r->effective_max_acmode,r->mailbox_ucb,r->assign_ops,&assigned);
    if(status==PORT_V37_CCB_OK)*channel=assigned;
    return status;
}
