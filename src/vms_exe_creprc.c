#include "vms_exe_creprc.h"
#include "vms_exe_gettim.h"
#include "vms_exe_setprv.h"
#include "vms_v37_dyn_tags.h"
#include "vms_v37_pcb.h"

static int has_privilege(uint64_t mask,unsigned bit){
    return bit<64u&&((mask>>bit)&UINT64_C(1))!=0u;
}

uint32_t port_exe_creprc_preflight(
    const struct port_exe_creprc_caller *caller,
    const struct port_exe_creprc_request *request,
    struct port_exe_creprc_result *result){
    if(!caller||!request||!result||caller->base_priority>=32u)
        return PORT_SS_BADPARAM;
    if(request->pid_output_address&&!request->pid_output_writable)
        return PORT_SS_ACCVIO;
    if(request->privilege_address&&!request->privilege_readable)
        return PORT_SS_ACCVIO;
    if(request->uic&&!has_privilege(caller->privilege_mask,
                                   PORT_V37_PRC_PRIV_DETACH))
        return PORT_SS_NOPRIV;
    if(request->status_flags&~(uint32_t)PORT_V37_PRC_STATUS_FLAGS)
        return PORT_SS_IVSTSFLG;

    struct port_exe_creprc_result candidate={0};
    candidate.status_flags=request->status_flags;
    if(request->uic){
        candidate.uic=request->uic;
        candidate.owner_pid=0u;
    }else{
        candidate.uic=caller->uic;
        candidate.owner_pid=caller->pid;
    }
    const uint8_t desired=(uint8_t)(request->base_priority&31u);
    uint8_t internal=(uint8_t)(31u-desired);
    if(!has_privilege(caller->privilege_mask,PORT_V37_PRC_PRIV_SETPRI)&&
       internal<caller->base_priority)
        internal=caller->base_priority;
    candidate.internal_priority=internal;

    uint64_t privileges=request->privilege_address?
        request->requested_privileges:caller->privilege_mask;
    if(request->privilege_address&&
       !has_privilege(caller->privilege_mask,PORT_V37_PRC_PRIV_SETPRV))
        privileges&=caller->privilege_mask;
    candidate.privilege_mask=privileges;
    static const uint8_t required_privilege[9]={
        0xffu,0xffu,PORT_V37_PRC_PRIV_PSWAPM,
        PORT_V37_PRC_PRIV_NOACNT,PORT_V37_PRC_PRIV_DETACH,
        0xffu,PORT_V37_PRC_PRIV_DETACH,PORT_V37_PRC_PRIV_DETACH,0xffu};
    for(unsigned bit=0;bit<9u;++bit){
        const uint8_t required=required_privilege[bit];
        if((request->status_flags&(UINT32_C(1)<<bit))&&required!=0xffu&&
           !has_privilege(candidate.privilege_mask,required))
            return PORT_SS_NOPRIV;
    }
    *result=candidate;
    return PORT_SS_NORMAL;
}

static uint32_t read32(const uint8_t *p){
    return (uint32_t)p[0]|((uint32_t)p[1]<<8)|
        ((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);
}
static uint16_t read16(const uint8_t *p){
    return (uint16_t)p[0]|((uint16_t)p[1]<<8);
}
static void write16(uint8_t *p,uint16_t x){
    p[0]=(uint8_t)x;p[1]=(uint8_t)(x>>8);
}
static void write32(uint8_t *p,uint32_t x){
    for(unsigned i=0u;i<4u;i++)p[i]=(uint8_t)(x>>(8u*i));
}
static void write64(uint8_t *p,uint64_t x){
    for(unsigned i=0u;i<8u;i++)p[i]=(uint8_t)(x>>(8u*i));
}

uint32_t port_exe_creprc_allocate_pcb(struct port_exe_np_allocator *allocator,
    struct port_exe_np_allocation *out){
    if(!allocator||!out)return PORT_EXE_CREPRC_ALLOC_BAD;
    struct port_exe_np_allocation candidate={0};
    const uint32_t status=port_exe_np_allocate(allocator,PORT_V37_PCB_BYTES,
        PORT_V37_DYN_PCB,&candidate);
    if(status==PORT_EXE_NP_POOL_EMPTY)return PORT_EXE_CREPRC_ALLOC_NOMEM;
    if(status!=PORT_EXE_NP_POOL_OK)
        return status==PORT_EXE_NP_POOL_CORRUPT?
            PORT_EXE_CREPRC_ALLOC_CORRUPT:PORT_EXE_CREPRC_ALLOC_BAD;
    if(!candidate.address||candidate.bytes<160u||
       ((uintptr_t)candidate.address&7u)||candidate.tag!=PORT_V37_DYN_PCB){
        if(port_exe_np_release(allocator,&candidate)!=PORT_EXE_NP_POOL_OK)
            return PORT_EXE_CREPRC_ALLOC_CORRUPT;
        return PORT_EXE_CREPRC_ALLOC_BAD;
    }
    uint8_t *const pcb=candidate.address;
    for(uint32_t i=PORT_V37_PCB_TYPE;i<PORT_V37_PCB_BYTES;i++)pcb[i]=0u;
    write16(pcb+PORT_V37_PCB_SIZE,160u);
    pcb[PORT_V37_PCB_BTYPE]=PORT_V37_DYN_PCB;
    pcb[11u]=0u;
    *out=candidate;
    return PORT_EXE_CREPRC_ALLOC_OK;
}

uint32_t port_exe_creprc_release_pcb(struct port_exe_np_allocator *allocator,
    const struct port_exe_np_allocation *allocation){
    if(!allocator||!allocation||!allocation->address||
       allocation->bytes<160u||allocation->tag!=PORT_V37_DYN_PCB)
        return PORT_EXE_CREPRC_ALLOC_BAD;
    const uint8_t *const pcb=allocation->address;
    if(read16(pcb+PORT_V37_PCB_SIZE)!=160u||
       pcb[PORT_V37_PCB_BTYPE]!=PORT_V37_DYN_PCB)
        return PORT_EXE_CREPRC_ALLOC_BAD;
    const uint32_t status=port_exe_np_release(allocator,allocation);
    return status==PORT_EXE_NP_POOL_OK?PORT_EXE_CREPRC_ALLOC_OK:
        (status==PORT_EXE_NP_POOL_CORRUPT||status==PORT_EXE_NP_POOL_DUP?
         PORT_EXE_CREPRC_ALLOC_CORRUPT:PORT_EXE_CREPRC_ALLOC_BAD);
}

uint32_t port_exe_creprc_allocate_pqb(struct port_exe_np_allocator *allocator,
    struct port_exe_np_allocation *out){
    if(!allocator||!out)return PORT_EXE_CREPRC_ALLOC_BAD;
    struct port_exe_np_allocation candidate={0};
    const uint32_t status=port_exe_np_allocate(allocator,PORT_V37_PQB_BYTES,
        PORT_V37_DYN_PQB,&candidate);
    if(status==PORT_EXE_NP_POOL_EMPTY)return PORT_EXE_CREPRC_ALLOC_NOMEM;
    if(status!=PORT_EXE_NP_POOL_OK)
        return status==PORT_EXE_NP_POOL_CORRUPT?
            PORT_EXE_CREPRC_ALLOC_CORRUPT:PORT_EXE_CREPRC_ALLOC_BAD;
    if(!candidate.address||candidate.bytes<PORT_V37_PQB_BYTES||
       ((uintptr_t)candidate.address&7u)||candidate.tag!=PORT_V37_DYN_PQB){
        if(port_exe_np_release(allocator,&candidate)!=PORT_EXE_NP_POOL_OK)
            return PORT_EXE_CREPRC_ALLOC_CORRUPT;
        return PORT_EXE_CREPRC_ALLOC_BAD;
    }
    uint8_t *const pqb=candidate.address;
    for(uint32_t i=PORT_V37_PQB_BODY;i<PORT_V37_PQB_BYTES;i++)pqb[i]=0u;
    write16(pqb+PORT_V37_PQB_SIZE,PORT_V37_PQB_BYTES);
    pqb[PORT_V37_PQB_TYPE]=PORT_V37_DYN_PQB;
    pqb[11u]=0u;
    *out=candidate;
    return PORT_EXE_CREPRC_ALLOC_OK;
}

uint32_t port_exe_creprc_release_pqb(struct port_exe_np_allocator *allocator,
    const struct port_exe_np_allocation *allocation){
    if(!allocator||!allocation||!allocation->address||
       allocation->bytes<PORT_V37_PQB_BYTES||
       allocation->tag!=PORT_V37_DYN_PQB)
        return PORT_EXE_CREPRC_ALLOC_BAD;
    const uint8_t *const pqb=allocation->address;
    if(read16(pqb+PORT_V37_PQB_SIZE)!=PORT_V37_PQB_BYTES||
       pqb[PORT_V37_PQB_TYPE]!=PORT_V37_DYN_PQB)
        return PORT_EXE_CREPRC_ALLOC_BAD;
    const uint32_t status=port_exe_np_release(allocator,allocation);
    return status==PORT_EXE_NP_POOL_OK?PORT_EXE_CREPRC_ALLOC_OK:
        (status==PORT_EXE_NP_POOL_CORRUPT||status==PORT_EXE_NP_POOL_DUP?
         PORT_EXE_CREPRC_ALLOC_CORRUPT:PORT_EXE_CREPRC_ALLOC_BAD);
}

uint32_t port_exe_creprc_set_pqb_system_username(uint8_t *pqb,
    size_t pqb_bytes){
    if(!pqb||pqb_bytes<PORT_V37_PQB_BYTES||
       read16(pqb+PORT_V37_PQB_SIZE)!=PORT_V37_PQB_BYTES||
       pqb[PORT_V37_PQB_TYPE]!=PORT_V37_DYN_PQB)
        return PORT_SS_BADPARAM;
    for(unsigned i=0u;i<PORT_V37_PQB_USERNAME_BYTES;i++)
        pqb[PORT_V37_PQB_USERNAME+i]=0x20u;
    static const uint8_t system_name[6]={'S','Y','S','T','E','M'};
    for(unsigned i=0u;i<sizeof system_name;i++)
        pqb[PORT_V37_PQB_USERNAME+i]=system_name[i];
    return PORT_SS_NORMAL;
}

uint32_t port_exe_creprc_apply_identity(uint8_t *pcb,size_t pcb_bytes,
    uint8_t *pqb,size_t pqb_bytes,const struct port_exe_creprc_result *result){
    if(!pcb||!pqb||!result||pcb_bytes<160u||
       pqb_bytes<PORT_V37_PQB_BYTES||result->internal_priority>=32u||
       read16(pcb+PORT_V37_PCB_SIZE)!=160u||
       pcb[PORT_V37_PCB_BTYPE]!=PORT_V37_DYN_PCB||
       read16(pqb+PORT_V37_PQB_SIZE)!=PORT_V37_PQB_BYTES||
       pqb[PORT_V37_PQB_TYPE]!=PORT_V37_DYN_PQB)
        return PORT_SS_BADPARAM;
    write64(pqb+PORT_V37_PQB_PRIVILEGES,result->privilege_mask);
    pcb[PORT_V37_PCB_PRI]=result->internal_priority;
    pcb[PORT_V37_PCB_PRIB]=result->internal_priority;
    write32(pcb+PORT_V37_PCB_UIC,result->uic);
    write32(pcb+PORT_V37_PCB_OWNER,result->owner_pid);
    return PORT_SS_NORMAL;
}

uint32_t port_exe_creprc_copy_process_name(uint8_t *pcb,size_t pcb_bytes,
    uint8_t *pqb,size_t pqb_bytes,const uint8_t *name,size_t name_bytes){
    if(!pcb||!pqb||pcb_bytes<160u||pqb_bytes<PORT_V37_PQB_BYTES||
       read16(pcb+PORT_V37_PCB_SIZE)!=160u||
       pcb[PORT_V37_PCB_BTYPE]!=PORT_V37_DYN_PCB||
       read16(pqb+PORT_V37_PQB_SIZE)!=PORT_V37_PQB_BYTES||
       pqb[PORT_V37_PQB_TYPE]!=PORT_V37_DYN_PQB)
        return PORT_SS_BADPARAM;
    if(name_bytes>15u)return PORT_SS_IVLOGNAM;
    if(name_bytes&&!name)return PORT_SS_ACCVIO;
    if(!name_bytes)return PORT_SS_NORMAL;
    pqb[PORT_V37_PQB_IMAGE]=(uint8_t)name_bytes;
    for(size_t i=0u;i<name_bytes;i++)
        pqb[PORT_V37_PQB_IMAGE+1u+i]=name[i];
    pcb[PORT_V37_PCB_LNAME]=(uint8_t)name_bytes;
    for(size_t i=0u;i<name_bytes;i++)
        pcb[PORT_V37_PCB_LNAME+1u+i]=name[i];
    for(unsigned i=0u;i<8u;i++)pqb[PORT_V37_PQB_IMAGE+i]=0u;
    return PORT_SS_NORMAL;
}

uint32_t port_exe_creprc_apply_quota_list(uint8_t *pqb,size_t pqb_bytes,
    const uint8_t *list,size_t list_bytes,
    const uint32_t defaults[PORT_V37_PQL_COUNT],
    const uint32_t minima[PORT_V37_PQL_COUNT]){
    if(!pqb||!defaults||!minima||pqb_bytes<PORT_V37_PQB_BYTES||
       read16(pqb+PORT_V37_PQB_SIZE)!=PORT_V37_PQB_BYTES||
       pqb[PORT_V37_PQB_TYPE]!=PORT_V37_DYN_PQB)
        return PORT_SS_BADPARAM;
    if(list_bytes&&!list)return PORT_SS_ACCVIO;
    uint32_t values[PORT_V37_PQL_COUNT];
    for(unsigned i=0u;i<PORT_V37_PQL_COUNT;i++)values[i]=defaults[i];
    size_t offset=0u;int terminated=0;
    while(offset<list_bytes){
        const uint8_t code=list[offset++];
        if(!code){terminated=1;break;}
        if(code>PORT_V37_PQL_COUNT)return PORT_SS_IVQUOTAL;
        if(list_bytes-offset<4u)return PORT_SS_ACCVIO;
        const uint32_t value=(uint32_t)list[offset]|
            ((uint32_t)list[offset+1u]<<8)|
            ((uint32_t)list[offset+2u]<<16)|
            ((uint32_t)list[offset+3u]<<24);
        offset+=4u;
        values[code-1u]=value;
    }
    if(list_bytes&&!terminated)return PORT_SS_ACCVIO;
    for(unsigned i=0u;i<PORT_V37_PQL_COUNT;i++)
        if((int32_t)values[i]<(int32_t)minima[i])values[i]=minima[i];
    for(unsigned i=0u;i<PORT_V37_PQL_COUNT;i++)
        write32(pqb+PORT_V37_PQB_QUOTAS+4u*i,values[i]);
    return PORT_SS_NORMAL;
}

uint32_t port_exe_creprc_apply_status_flags(uint8_t *pcb,size_t pcb_bytes,
    const struct port_exe_creprc_result *result){
    if(!pcb||!result||pcb_bytes<PORT_V37_PCB_BYTES||
       read16(pcb+PORT_V37_PCB_SIZE)!=160u||
       pcb[PORT_V37_PCB_BTYPE]!=PORT_V37_DYN_PCB)
        return PORT_SS_BADPARAM;
    const uint32_t flags=result->status_flags;
    if(flags&~(uint32_t)PORT_V37_PRC_STATUS_FLAGS)
        return PORT_SS_IVSTSFLG;
    static const uint8_t pcb_bit[9]={10u,9u,4u,15u,14u,19u,20u,21u,24u};
    static const uint8_t privilege[9]={0xffu,0xffu,
        PORT_V37_PRC_PRIV_PSWAPM,PORT_V37_PRC_PRIV_NOACNT,
        PORT_V37_PRC_PRIV_DETACH,0xffu,PORT_V37_PRC_PRIV_DETACH,
        PORT_V37_PRC_PRIV_DETACH,0xffu};
    for(unsigned i=0u;i<9u;i++){
        if(!(flags&(UINT32_C(1)<<i)))continue;
        const uint8_t required=privilege[i];
        if(required!=0xffu&&!has_privilege(result->privilege_mask,required))
            return PORT_SS_NOPRIV;
    }
    uint32_t status=read32(pcb+PORT_V37_PCB_STS);
    for(unsigned i=0u;i<9u;i++)
        if(flags&(UINT32_C(1)<<i))status|=UINT32_C(1)<<pcb_bit[i];
    write32(pcb+PORT_V37_PCB_STS,status);
    return PORT_SS_NORMAL;
}

uint32_t port_exe_creprc_init_pcb_queues(uint8_t *pcb,size_t pcb_bytes,
    uint32_t pcb_guest_address,uint16_t mailbox_unit,uint16_t shell_pages){
    if(!pcb||pcb_bytes<160u||!pcb_guest_address||
       (pcb_guest_address&3u)||pcb_guest_address>UINT32_MAX-160u||
       read16(pcb+PORT_V37_PCB_SIZE)!=160u||
       pcb[PORT_V37_PCB_BTYPE]!=PORT_V37_DYN_PCB)
        return PORT_SS_BADPARAM;
    write32(pcb+PORT_V37_PCB_ASTQFL,
        pcb_guest_address+PORT_V37_PCB_ASTQFL);
    write32(pcb+PORT_V37_PCB_ASTQBL,
        pcb_guest_address+PORT_V37_PCB_ASTQFL);
    pcb[PORT_V37_PCB_ASTEN]=0x0fu;
    write32(pcb+PORT_V37_PCB_LOCKQFL,
        pcb_guest_address+PORT_V37_PCB_LOCKQFL);
    write32(pcb+PORT_V37_PCB_LOCKQBL,
        pcb_guest_address+PORT_V37_PCB_LOCKQFL);
    write32(pcb+PORT_V37_PCB_ARB,
        pcb_guest_address+PORT_V37_PCB_PRIV);
    write16(pcb+PORT_V37_PCB_TMBU,mailbox_unit);
    write16(pcb+PORT_V37_PCB_PPGCNT,shell_pages);
    return PORT_SS_NORMAL;
}

uint32_t port_exe_creprc_activate_slot(struct port_ini_procvec *vector,
    uint32_t null_pcb_address,uint32_t new_pcb_address,
    uint8_t *new_pcb,size_t new_pcb_bytes,
    uint16_t *process_count,uint16_t process_limit,uint32_t *pid){
    if(!vector||!vector->pcb_vector||!vector->sequence_vector||
       !null_pcb_address||!new_pcb_address||(new_pcb_address&3u)||
       !new_pcb||new_pcb_bytes<PORT_V37_PCB_BYTES||
       read16(new_pcb+PORT_V37_PCB_SIZE)!=PORT_V37_PCB_BYTES||
       new_pcb[PORT_V37_PCB_BTYPE]!=PORT_V37_DYN_PCB||
       read32(new_pcb)!=new_pcb_address||
       !process_count||!pid||vector->max_pix<1u||
       vector->system_pix!=vector->max_pix+1u||
       vector->allocated_slots!=vector->system_pix+1u)
        return PORT_EXE_CREPRC_SLOT_BAD;

    uint32_t pix=vector->max_pix;
    for(;;){
        if(read32(vector->pcb_vector+pix*4u)==null_pcb_address)break;
        if(pix==1u)return PORT_EXE_CREPRC_SLOT_EXHAUSTED;
        --pix;
    }
    if(*process_count>=process_limit)
        return PORT_EXE_CREPRC_SLOT_EXHAUSTED;

    uint16_t sequence=(uint16_t)(read16(vector->sequence_vector+pix*2u)+1u);
    if(sequence&0x8000u)sequence=0u;
    const uint32_t candidate_pid=((uint32_t)sequence<<16)|pix;
    write16(vector->sequence_vector+pix*2u,sequence);
    write32(new_pcb+PORT_V37_PCB_PID,candidate_pid);
    write32(vector->pcb_vector+pix*4u,new_pcb_address);
    ++*process_count;
    *pid=candidate_pid;
    return PORT_EXE_CREPRC_SLOT_OK;
}

uint32_t port_exe_creprc_activate_native(
    struct port_v37_scheduler *scheduler,struct port_ini_procvec *vector,
    uint32_t null_pcb_address,uint32_t new_pcb_address,uint8_t *new_pcb,
    size_t new_pcb_bytes,uint32_t phd_address,uint32_t restore_frame_address,
    const struct port_v37_phd_seed *seed,uint8_t priority_increment_class,
    uint16_t *process_count,uint16_t process_limit,uint32_t *pid,
    uint32_t *scheduler_status){
    if(scheduler_status)*scheduler_status=PORT_V37_SCHED_BADPARAM;
    if(!scheduler||!vector||!new_pcb||!seed||!process_count||!pid||
       !phd_address||phd_address>UINT32_MAX-PORT_V37_PHD_PCB||
       !restore_frame_address||priority_increment_class>=
           PORT_V37_SCH_INCREMENT_CLASSES||
       new_pcb_bytes<PORT_V37_PCB_BYTES||
       read16(new_pcb+PORT_V37_PCB_SIZE)!=PORT_V37_PCB_BYTES||
       new_pcb[PORT_V37_PCB_BTYPE]!=PORT_V37_DYN_PCB||
       read32(new_pcb)!=new_pcb_address||
       port_v37_sched_check(scheduler)!=PORT_V37_SCHED_OK)
        return PORT_EXE_CREPRC_RUN_BAD;
    const uint16_t old_count=*process_count;
    const uint32_t old_pid=read32(new_pcb+PORT_V37_PCB_PID);
    uint32_t candidate_pid=0u;
    const uint32_t activated=port_exe_creprc_activate_slot(vector,
        null_pcb_address,new_pcb_address,new_pcb,new_pcb_bytes,
        process_count,process_limit,&candidate_pid);
    if(activated!=PORT_EXE_CREPRC_SLOT_OK)return PORT_EXE_CREPRC_RUN_SLOT;

    /* The process vector must publish the PID before CHSE can request a
     * reschedule. Scheduler-start failure is limited to an invalid supplied
     * PHD/frame or an inconsistent queue; restore the unpublished PCB slot,
     * but retain the advanced sequence number to prevent PID reuse. */
    const uint32_t started=port_v37_sched_start_context(scheduler,
        new_pcb_address,phd_address,phd_address+PORT_V37_PHD_PCB,
        restore_frame_address,seed,priority_increment_class);
    if(started!=PORT_V37_SCHED_OK){
        const uint16_t pix=(uint16_t)candidate_pid;
        if(port_ini_procvec_ptr(vector,pix)!=new_pcb_address||
           port_ini_procvec_set_pcb(vector,pix,null_pcb_address)!=
               PORT_INI_PROC_OK||*process_count!=old_count+1u){
            if(scheduler_status)*scheduler_status=PORT_V37_SCHED_CORRUPT;
            return PORT_EXE_CREPRC_RUN_SCHED;
        }
        *process_count=old_count;
        write32(new_pcb+PORT_V37_PCB_PID,old_pid);
        if(scheduler_status)*scheduler_status=started;
        return PORT_EXE_CREPRC_RUN_SCHED;
    }
    *pid=candidate_pid;
    if(scheduler_status)*scheduler_status=PORT_V37_SCHED_OK;
    return PORT_EXE_CREPRC_RUN_OK;
}
