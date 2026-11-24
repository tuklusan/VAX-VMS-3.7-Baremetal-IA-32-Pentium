#include "vms_sysassign.h"
#include "vms_exe_io.h"
#include "vms_v37_pcb.h"

enum { ASSIGN_STAGE_VALIDATE,ASSIGN_STAGE_SEARCH,ASSIGN_STAGE_CCB };
static uint32_t sysassign_local(
    const struct port_v37_sysassign_local_request *r,uint16_t *channel,
    unsigned *stage){
    if(!channel)return PORT_V37_CCB_BAD;
    *channel=0u; /* SYSASSIGN clears the caller's channel before lookup. */
    if(!r||!r->device_database||!r->devices||!r->device_name||
       !r->device_name_bytes||!r->ccb_base||!r->high_water_channel||
       !r->channel_count||!r->process_pid||r->effective_max_acmode>3u||
       !r->assign_ops||
       r->device_database_base!=(uint32_t)(uintptr_t)r->device_database)
        return PORT_V37_CCB_BAD;
    if(stage)*stage=ASSIGN_STAGE_SEARCH;
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
    if(stage)*stage=ASSIGN_STAGE_CCB;
    uint16_t assigned=0u;
    const uint32_t status=port_v37_ccb_assign_local(r->ccb_base,
        r->ccb_preceding_bytes,r->channel_count,r->high_water_channel,ucb,
        r->device_database_bytes-offset,found.ucb,r->process_pid,r->arb_va,
        r->effective_max_acmode,r->mailbox_ucb,r->assign_ops,&assigned);
    if(status==PORT_V37_CCB_OK)*channel=assigned;
    return status;
}

uint32_t port_v37_sysassign_local(
    const struct port_v37_sysassign_local_request *r,uint16_t *channel){
    return sysassign_local(r,channel,0);
}

uint32_t port_v37_sysassign_descriptor(
    const struct port_v37_sysassign_descriptor_request *r,uint16_t *channel){
    if(!channel)return PORT_V37_CCB_BAD;
    *channel=0u;
    if(!r||!r->local||!r->memory||r->caller_access_mode>3u)
        return PORT_V37_CCB_BAD;
    if(!r->device_name_descriptor)return PORT_SS_IVDEVNAM;
    uint8_t descriptor[8];
    if(!port_exe_probe_read(r->memory,r->device_name_descriptor,
                            sizeof descriptor)||
       !port_exe_read(r->memory,r->device_name_descriptor,descriptor,
                      sizeof descriptor))return PORT_SS_ACCVIO;
    const uint32_t length=(uint32_t)descriptor[0]|
                          ((uint32_t)descriptor[1]<<8);
    const uint32_t address=(uint32_t)descriptor[4]|
        ((uint32_t)descriptor[5]<<8)|((uint32_t)descriptor[6]<<16)|
        ((uint32_t)descriptor[7]<<24);
    if(!length||length>PORT_V37_IOC_NAME_MAX)return PORT_SS_IVLOGNAM;
    if(!address)return PORT_SS_ACCVIO;
    char name[PORT_V37_IOC_NAME_MAX];
    if(!port_exe_probe_read(r->memory,address,length)||
       !port_exe_read(r->memory,address,name,length))
        return PORT_SS_ACCVIO;
    struct port_v37_sysassign_local_request local=*r->local;
    if(r->process){
        const struct port_v37_sysqio_process *const process=r->process;
        if(!process->source_pcb||
           process->source_pcb_bytes<PORT_V37_PCB_BYTES||!process->pid||
           port_v37_read32(process->source_pcb+PORT_V37_PCB_PID)!=
               process->pid||!process->ccb_base||
           !process->ccb_channel_count||
           (size_t)process->ccb_channel_count>
               process->ccb_preceding_bytes/PORT_V37_CCB_BYTES||
           process->ccb_high_water>
               process->ccb_channel_count*PORT_V37_CCB_BYTES)
            return PORT_V37_CCB_BAD;
        local.ccb_base=process->ccb_base;
        local.ccb_preceding_bytes=process->ccb_preceding_bytes;
        local.channel_count=process->ccb_channel_count;
        local.high_water_channel=&r->process->ccb_high_water;
        local.process_pid=process->pid;
        local.arb_va=process->arb_va;
    }
    const uint8_t requested=(uint8_t)(r->requested_access_mode&3u);
    local.effective_max_acmode=requested>r->caller_access_mode?
        requested:r->caller_access_mode;
    local.device_name=name;local.device_name_bytes=length;
    unsigned stage=ASSIGN_STAGE_VALIDATE;
    uint32_t status=sysassign_local(&local,channel,&stage);
    if(stage==ASSIGN_STAGE_SEARCH){
        if(status==PORT_V37_IOC_BAD)return PORT_SS_IVDEVNAM;
        if(status==PORT_V37_IOC_NO_SUCH)return PORT_SS_NOSUCHDEV;
    }else if(stage==ASSIGN_STAGE_CCB){
        if(status==PORT_V37_CCB_NOCHANNEL)return PORT_SS_NOIOCHAN;
        if(status==PORT_V37_CCB_NOPRIV||
           status==PORT_V37_ASSIGN_NOPRIV)return PORT_SS_NOPRIV;
        if(status==PORT_V37_ASSIGN_DEVALLOC)return PORT_SS_DEVALLOC;
    }
    return status;
}

uint32_t port_v37_sysassign_descriptor_service(
    const struct port_v37_sysassign_descriptor_request *r,
    uint32_t channel_output){
    if(!r||!r->memory)return PORT_V37_CCB_BAD;
    uint8_t output[2]={0u,0u};
    if(!port_exe_probe_write(r->memory,channel_output,sizeof output)||
       !port_exe_write(r->memory,channel_output,output,sizeof output))
        return PORT_SS_ACCVIO;
    uint16_t channel=0u;
    const uint32_t status=port_v37_sysassign_descriptor(r,&channel);
    if(!(status&1u))return status;
    output[0]=(uint8_t)channel;output[1]=(uint8_t)(channel>>8);
    if(!port_exe_write(r->memory,channel_output,output,sizeof output))
        return PORT_SS_ACCVIO;
    return status;
}

int port_v37_sysassign_lookup_qio_channel(void *context,uint32_t pid,
    uint32_t channel,struct port_v37_sysqio_resolved_channel *out){
    struct port_v37_sysassign_qio_context *c=context;
    if(!c||!out||!pid||pid!=c->owner_pid||!c->owner_pid||
       !c->ccb_base||!c->device_database||!c->device||
       !c->assigned_ucb||c->device_database_base!=
           (uint32_t)(uintptr_t)c->device_database||
       c->assigned_ucb<c->device_database_base)return -1;
    struct port_v37_ccb_view view={0};
    if(port_v37_ccb_resolve(c->ccb_base,c->ccb_preceding_bytes,channel,
          c->maximum_channel_offset,0u,&view)!=PORT_V37_CCB_OK||
       view.ucb!=c->assigned_ucb)return -1;
    const uint32_t offset=view.ucb-c->device_database_base;
    if(offset>c->device_database_bytes||
       PORT_V37_UCB_FIXED_BYTES>c->device_database_bytes-offset)
        return -1;
    const struct port_v37_sysqio_resolved_channel result={
        .device=c->device,
        .ucb=(void *)(uintptr_t)(c->device_database+offset),
        .window=view.window,.ccb_access_limit=view.access_limit};
    *out=result;
    return 0;
}

uint32_t port_v37_sysassign_qio_charge(void *context,uint32_t pid,
    uint32_t channel,uint8_t access_mode,const struct port_v37_assign_ops *ops,
    uint16_t *outstanding){
    struct port_v37_sysassign_qio_context *c=context;
    if(!c||!ops||!ops->lock_io_database||!ops->unlock_io_database||
       !outstanding||!pid||pid!=c->owner_pid||access_mode>3u||!c->ccb_base||
       !c->assigned_ucb)return PORT_V37_CCB_BAD;
    const uint32_t locked=ops->lock_io_database(ops->context);
    if(!(locked&1u))return locked;
    struct port_v37_ccb_view view={0};
    uint32_t status=port_v37_ccb_resolve(c->ccb_base,
        c->ccb_preceding_bytes,channel,c->maximum_channel_offset,access_mode,
        &view);
    if(status==PORT_V37_CCB_OK&&view.ucb!=c->assigned_ucb)
        status=PORT_V37_CCB_IVCHAN;
    if(status==PORT_V37_CCB_OK){
        uint8_t *const count=view.ccb+PORT_V37_CCB_IOC;
        const uint16_t next=(uint16_t)((uint16_t)count[0]|
            ((uint16_t)count[1]<<8));
        const uint16_t charged=(uint16_t)(next+1u);
        count[0]=(uint8_t)charged;count[1]=(uint8_t)(charged>>8);
        *outstanding=charged;
    }
    ops->unlock_io_database(ops->context);
    return status;
}
