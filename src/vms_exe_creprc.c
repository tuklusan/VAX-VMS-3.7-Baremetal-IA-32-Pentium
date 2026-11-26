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

uint32_t port_exe_creprc_activate_slot(struct port_ini_procvec *vector,
    uint32_t null_pcb_address,uint32_t new_pcb_address,
    uint16_t *process_count,uint16_t process_limit,uint32_t *pid){
    if(!vector||!vector->pcb_vector||!vector->sequence_vector||
       !null_pcb_address||!new_pcb_address||(new_pcb_address&3u)||
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
    write32(vector->pcb_vector+pix*4u,new_pcb_address);
    ++*process_count;
    *pid=candidate_pid;
    return PORT_EXE_CREPRC_SLOT_OK;
}
