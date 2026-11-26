#include "vms_exe_creprc.h"
#include "vms_exe_gettim.h"
#include "vms_exe_setprv.h"
#include "vms_v37_dyn_tags.h"
#include "vms_v37_pcb.h"
#include <stdio.h>
#include <string.h>

static int fail(const char *name){fprintf(stderr,"CREPRC FAIL: %s\n",name);return 1;}
static uint32_t test_get32(const uint8_t *p){
    return (uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|
        ((uint32_t)p[3]<<24);
}
static uint32_t test_raise_ipl(void *opaque){
    uint32_t *ipl=opaque;uint32_t previous=*ipl;*ipl=2u;return previous;
}
static void test_restore_ipl(void *opaque,uint32_t previous){
    *(uint32_t *)opaque=previous;
}
int main(void){
    const struct port_exe_creprc_caller caller={
        .pid=0x00010023u,.uic=0x00020020u,.base_priority=15u,
        .privilege_mask=0u};
    struct port_exe_creprc_result result={0};
    struct port_exe_creprc_request request={.base_priority=24u};
    if(port_exe_creprc_preflight(&caller,&request,&result)!=PORT_SS_NORMAL||
       result.uic!=caller.uic||result.owner_pid!=caller.pid||
       result.internal_priority!=15u||result.privilege_mask!=caller.privilege_mask)
        return fail("subprocess owner/UIC inheritance, priority and privilege defaults");

    request=(struct port_exe_creprc_request){.pid_output_address=0x1000u,
        .pid_output_writable=1u,.privilege_address=0x2000u,
        .privilege_readable=1u,.requested_privileges=UINT64_MAX,
        .base_priority=31u,.status_flags=0x123u};
    if(port_exe_creprc_preflight(&caller,&request,&result)!=PORT_SS_NORMAL||
       result.internal_priority!=15u||result.privilege_mask!=caller.privilege_mask||
       result.status_flags!=0x123u||result.owner_pid!=caller.pid)
        return fail("requested priority/privilege minimized for unprivileged subprocess");

    struct port_exe_creprc_caller privileged=caller;
    privileged.privilege_mask|=(UINT64_C(1)<<PORT_V37_PRC_PRIV_SETPRI)|
        (UINT64_C(1)<<PORT_V37_PRC_PRIV_SETPRV);
    if(port_exe_creprc_preflight(&privileged,&request,&result)!=PORT_SS_NORMAL||
       result.internal_priority!=0u||result.privilege_mask!=UINT64_MAX)
        return fail("ALTPRI and SETPRV bypass source minimization");

    request.uic=0x00030030u;
    struct port_exe_creprc_result before=result;
    if(port_exe_creprc_preflight(&caller,&request,&result)!=PORT_SS_NOPRIV||
       memcmp(&result,&before,sizeof result))
        return fail("detached UIC requires DETACH without output mutation");
    privileged.privilege_mask|=UINT64_C(1)<<PORT_V37_PRC_PRIV_DETACH;
    if(port_exe_creprc_preflight(&privileged,&request,&result)!=PORT_SS_NORMAL||
       result.uic!=request.uic||result.owner_pid!=0u)
        return fail("DETACH-authorized independent process identity");

    request.status_flags=0x200u;before=result;
    if(port_exe_creprc_preflight(&privileged,&request,&result)!=PORT_SS_IVSTSFLG||
       memcmp(&result,&before,sizeof result))
        return fail("status flag MBZ validation is transactional");
    request.uic=0u;request.status_flags=1u<<4;before=result;
    if(port_exe_creprc_preflight(&caller,&request,&result)!=PORT_SS_NOPRIV||
       memcmp(&result,&before,sizeof result))
        return fail("BATCH status requires DETACH in granted process privileges");
    request.status_flags=1u<<2;before=result;
    request.requested_privileges=0u;
    if(port_exe_creprc_preflight(&privileged,&request,&result)!=PORT_SS_NOPRIV||
       memcmp(&result,&before,sizeof result))
        return fail("PSWAPM status requires PSWAPM in granted privileges");
    privileged.privilege_mask|=UINT64_C(1)<<PORT_V37_PRC_PRIV_PSWAPM;
    request.status_flags=1u<<2;
    request.requested_privileges=UINT64_C(1)<<PORT_V37_PRC_PRIV_PSWAPM;
    if(port_exe_creprc_preflight(&privileged,&request,&result)!=PORT_SS_NORMAL)
        return fail("authorized PSWAPM status flag");
    request.status_flags=0u;request.pid_output_writable=0u;before=result;
    if(port_exe_creprc_preflight(&privileged,&request,&result)!=PORT_SS_ACCVIO||
       memcmp(&result,&before,sizeof result))
        return fail("PID output probe");
    request.pid_output_writable=1u;request.privilege_readable=0u;before=result;
    if(port_exe_creprc_preflight(&privileged,&request,&result)!=PORT_SS_ACCVIO||
       memcmp(&result,&before,sizeof result))
        return fail("requested privilege mask probe");
    struct port_exe_creprc_caller malformed=caller;malformed.base_priority=32u;
    if(port_exe_creprc_preflight(&malformed,&request,&result)!=PORT_SS_BADPARAM)
        return fail("source PCB priority validation");
    _Alignas(8) uint8_t pcb_pool_memory[1024];
    uint32_t pool_ipl=0u;
    const struct port_v37_irp_pool_sync pool_sync={
        &pool_ipl,test_raise_ipl,test_restore_ipl};
    struct port_exe_np_pool pcb_pool={0};
    struct port_exe_np_allocator pcb_allocator={
        .variable=&pcb_pool,.sync=&pool_sync};
    struct port_exe_np_allocation pcb_allocation={0};
    if(port_exe_np_pool_init(&pcb_pool,pcb_pool_memory,
          sizeof pcb_pool_memory,&pool_sync)!=PORT_EXE_NP_POOL_OK||
       port_exe_creprc_allocate_pcb(&pcb_allocator,&pcb_allocation)!=
          PORT_EXE_CREPRC_ALLOC_OK||!pcb_allocation.address||
       pcb_allocation.bytes!=160u||pool_ipl!=0u)
        return fail("EXE$ALLOCPCB source pool allocation, size and IPL");
    const uint8_t *const pcb_bytes=pcb_allocation.address;
    if(pcb_bytes[8u]!=160u||pcb_bytes[9u]||
       pcb_bytes[10u]!=PORT_V37_DYN_PCB||pcb_bytes[11u])
        return fail("DYN PCB size and type fields");
    for(unsigned i=PORT_V37_PCB_TYPE;i<PORT_V37_PCB_BYTES;i++)
        if(pcb_bytes[i])return fail("SYSCREPRC zeroes the fixed PCB body");
    struct port_exe_creprc_result identity_result={
        .uic=0x12345678u,.owner_pid=0x00020023u,
        .internal_priority=7u,.privilege_mask=UINT64_C(0x1122334455667788)};
    if(port_exe_creprc_apply_identity(pcb_allocation.address,
          pcb_allocation.bytes,NULL,0u,&identity_result)!=PORT_SS_BADPARAM)
        return fail("identity application requires both allocated blocks");
    struct port_exe_creprc_result flag_result={
        .status_flags=PORT_V37_PRC_STATUS_FLAGS,
        .privilege_mask=(UINT64_C(1)<<PORT_V37_PRC_PRIV_PSWAPM)|
            (UINT64_C(1)<<PORT_V37_PRC_PRIV_NOACNT)|
            (UINT64_C(1)<<PORT_V37_PRC_PRIV_DETACH)};
    if(port_exe_creprc_apply_status_flags(pcb_allocation.address,
          pcb_allocation.bytes,&flag_result)!=PORT_SS_NORMAL)
        return fail("SYSCREPRC status flag translation");
    uint32_t expected_flags=(UINT32_C(1)<<10)|(UINT32_C(1)<<9)|
        (UINT32_C(1)<<4)|(UINT32_C(1)<<15)|(UINT32_C(1)<<14)|
        (UINT32_C(1)<<19)|(UINT32_C(1)<<20)|(UINT32_C(1)<<21)|
        (UINT32_C(1)<<24);
    if(pcb_bytes[PORT_V37_PCB_STS]!=(uint8_t)expected_flags||
       pcb_bytes[PORT_V37_PCB_STS+1u]!=(uint8_t)(expected_flags>>8)||
       pcb_bytes[PORT_V37_PCB_STS+2u]!=(uint8_t)(expected_flags>>16)||
       pcb_bytes[PORT_V37_PCB_STS+3u]!=(uint8_t)(expected_flags>>24))
        return fail("all nine source status to PCB bits");
    if(port_exe_creprc_apply_status_flags(pcb_allocation.address,
          pcb_allocation.bytes,&flag_result)!=PORT_SS_NORMAL||
       pcb_bytes[PORT_V37_PCB_STS]!=(uint8_t)expected_flags)
        return fail("status translation is idempotent");
    flag_result.privilege_mask=0u;
    const uint32_t old_status=pcb_bytes[PORT_V37_PCB_STS];
    if(port_exe_creprc_apply_status_flags(pcb_allocation.address,
          pcb_allocation.bytes,&flag_result)!=PORT_SS_NOPRIV||
       pcb_bytes[PORT_V37_PCB_STS]!=old_status)
        return fail("status privilege rejection leaves PCB unchanged");
    flag_result.status_flags=1u<<9;
    flag_result.privilege_mask=UINT64_MAX;
    if(port_exe_creprc_apply_status_flags(pcb_allocation.address,
          pcb_allocation.bytes,&flag_result)!=PORT_SS_IVSTSFLG||
       pcb_bytes[PORT_V37_PCB_STS]!=old_status)
        return fail("reserved source status flag rejection is atomic");
    if(port_exe_creprc_init_pcb_queues(pcb_allocation.address,
          pcb_allocation.bytes,0x12345000u,0x0421u,17u)!=PORT_SS_NORMAL||
       test_get32(pcb_bytes+PORT_V37_PCB_ASTQFL)!=0x12345000u+
          PORT_V37_PCB_ASTQFL||
       test_get32(pcb_bytes+PORT_V37_PCB_ASTQBL)!=0x12345000u+
          PORT_V37_PCB_ASTQFL||pcb_bytes[PORT_V37_PCB_ASTEN]!=0x0fu||
       test_get32(pcb_bytes+PORT_V37_PCB_LOCKQFL)!=0x12345000u+
          PORT_V37_PCB_LOCKQFL||
       test_get32(pcb_bytes+PORT_V37_PCB_LOCKQBL)!=0x12345000u+
          PORT_V37_PCB_LOCKQFL||
       test_get32(pcb_bytes+PORT_V37_PCB_ARB)!=0x12345000u+
          PORT_V37_PCB_PRIV||
       pcb_bytes[PORT_V37_PCB_TMBU]!=0x21u||
       pcb_bytes[PORT_V37_PCB_TMBU+1u]!=0x04u||
       pcb_bytes[PORT_V37_PCB_PPGCNT]!=17u||
       pcb_bytes[PORT_V37_PCB_PPGCNT+1u])
        return fail("SYSCREPRC self-linked queues, ARB, mailbox and shell pages");
    uint8_t pcb_before[160];memcpy(pcb_before,pcb_bytes,sizeof pcb_before);
    if(port_exe_creprc_init_pcb_queues(pcb_allocation.address,
          pcb_allocation.bytes,UINT32_MAX-159u,0u,0u)!=PORT_SS_BADPARAM||
       memcmp(pcb_before,pcb_bytes,sizeof pcb_before))
        return fail("PCB queue initialization rejects address wrap atomically");
    struct port_exe_np_allocation pqb_allocation={0};
    if(port_exe_creprc_allocate_pqb(&pcb_allocator,&pqb_allocation)!=
          PORT_EXE_CREPRC_ALLOC_OK||!pqb_allocation.address||
       pqb_allocation.bytes!=PORT_V37_PQB_BYTES||pool_ipl!=0u)
        return fail("EXE$ALLOCPQB source pool allocation and IPL");
    const uint8_t *const pqb_bytes=pqb_allocation.address;
    if(pqb_bytes[PORT_V37_PQB_SIZE]!=0xf0u||
       pqb_bytes[PORT_V37_PQB_SIZE+1u]!=0x01u||
       pqb_bytes[PORT_V37_PQB_TYPE]!=PORT_V37_DYN_PQB||
       pqb_bytes[11u])return fail("DYN PQB header");
    for(unsigned i=PORT_V37_PQB_BODY;i<PORT_V37_PQB_BYTES;i++)
        if(pqb_bytes[i])return fail("SYSCREPRC zeroes fixed PQB body");
    if(port_exe_creprc_apply_identity(pcb_allocation.address,
          pcb_allocation.bytes,pqb_allocation.address,pqb_allocation.bytes,
          &identity_result)!=PORT_SS_NORMAL||
       test_get32(pcb_bytes+PORT_V37_PCB_OWNER)!=identity_result.owner_pid||
       test_get32(pcb_bytes+PORT_V37_PCB_UIC)!=identity_result.uic||
       pcb_bytes[PORT_V37_PCB_PRI]!=identity_result.internal_priority||
       pcb_bytes[PORT_V37_PCB_PRIB]!=identity_result.internal_priority||
       test_get32(pqb_bytes+PORT_V37_PQB_PRIVILEGES)!=
          (uint32_t)identity_result.privilege_mask||
       test_get32(pqb_bytes+PORT_V37_PQB_PRIVILEGES+4u)!=
          (uint32_t)(identity_result.privilege_mask>>32))
        return fail("SYSCREPRC PQB grants, PCB priority, UIC and owner fields");
    static const uint8_t process_name[15]="ABCDEFGHIJKLMNO";
    if(port_exe_creprc_copy_process_name(pcb_allocation.address,
          pcb_allocation.bytes,pqb_allocation.address,pqb_allocation.bytes,
          process_name,sizeof process_name)!=PORT_SS_NORMAL||
       pcb_bytes[PORT_V37_PCB_LNAME]!=sizeof process_name||
       memcmp(pcb_bytes+PORT_V37_PCB_LNAME+1u,process_name,
          sizeof process_name))
        return fail("SYSCREPRC MOVSTRING process name into PCB LNAME");
    for(unsigned i=0u;i<8u;i++)
        if(pqb_bytes[PORT_V37_PQB_IMAGE+i])
            return fail("SYSCREPRC clears transient PQB image name");
    uint8_t pcb_snapshot[160],pqb_snapshot[PORT_V37_PQB_BYTES];
    memcpy(pcb_snapshot,pcb_bytes,sizeof pcb_snapshot);
    memcpy(pqb_snapshot,pqb_bytes,sizeof pqb_snapshot);
    if(port_exe_creprc_copy_process_name(pcb_allocation.address,
          pcb_allocation.bytes,pqb_allocation.address,pqb_allocation.bytes,
          process_name,16u)!=PORT_SS_IVLOGNAM||
       memcmp(pcb_snapshot,pcb_bytes,sizeof pcb_snapshot)||
       memcmp(pqb_snapshot,pqb_bytes,sizeof pqb_snapshot)||
       port_exe_creprc_copy_process_name(pcb_allocation.address,
          pcb_allocation.bytes,pqb_allocation.address,pqb_allocation.bytes,
          NULL,1u)!=PORT_SS_ACCVIO||
       memcmp(pcb_snapshot,pcb_bytes,sizeof pcb_snapshot)||
       memcmp(pqb_snapshot,pqb_bytes,sizeof pqb_snapshot))
        return fail("SYSCREPRC process name invalid length/access atomicity");
    if(port_exe_creprc_set_pqb_system_username(pqb_allocation.address,
          pqb_allocation.bytes)!=PORT_SS_NORMAL||
       memcmp(pqb_bytes+PORT_V37_PQB_USERNAME,"SYSTEM      ",12u))
        return fail("SYSCREPRC fixed-width SYSTEM username default");
    static const uint32_t quota_defaults[PORT_V37_PQL_COUNT]={
        6u,6u,8192u,0u,6u,16u,2048u,8u,8u,200u,100u,20u,200u};
    static const uint32_t quota_minima[PORT_V37_PQL_COUNT]={
        2u,2u,1024u,0u,2u,2u,256u,0u,0u,10u,10u,2u,10u};
    static const uint8_t quota_list[]={
        1u,1u,0u,0u,0u, 1u,7u,0u,0u,0u, 2u,1u,0u,0u,0u,
        13u,44u,1u,0u,0u, 0u};
    static const uint32_t quota_expected[PORT_V37_PQL_COUNT]={
        7u,2u,8192u,0u,6u,16u,2048u,8u,8u,200u,100u,20u,300u};
    if(port_exe_creprc_apply_quota_list(pqb_allocation.address,
          pqb_allocation.bytes,quota_list,sizeof quota_list,
          quota_defaults,quota_minima)!=PORT_SS_NORMAL)
        return fail("SYSCREPRC quota list/default/minimum processing");
    for(unsigned i=0u;i<PORT_V37_PQL_COUNT;i++)
        if(test_get32(pqb_bytes+PORT_V37_PQB_QUOTAS+4u*i)!=
           quota_expected[i])return fail("PQL duplicate-last, minimum and defaults");
    memcpy(pqb_snapshot,pqb_bytes,sizeof pqb_snapshot);
    static const uint8_t bad_quota[]={14u,1u,0u,0u,0u,0u};
    static const uint8_t truncated_quota[]={1u,1u};
    static const uint8_t unterminated_quota[]={1u,1u,0u,0u,0u};
    if(port_exe_creprc_apply_quota_list(pqb_allocation.address,
          pqb_allocation.bytes,bad_quota,sizeof bad_quota,
          quota_defaults,quota_minima)!=PORT_SS_IVQUOTAL||
       memcmp(pqb_snapshot,pqb_bytes,sizeof pqb_snapshot)||
       port_exe_creprc_apply_quota_list(pqb_allocation.address,
          pqb_allocation.bytes,truncated_quota,sizeof truncated_quota,
          quota_defaults,quota_minima)!=PORT_SS_ACCVIO||
       memcmp(pqb_snapshot,pqb_bytes,sizeof pqb_snapshot)||
       port_exe_creprc_apply_quota_list(pqb_allocation.address,
          pqb_allocation.bytes,unterminated_quota,sizeof unterminated_quota,
          quota_defaults,quota_minima)!=PORT_SS_ACCVIO||
       memcmp(pqb_snapshot,pqb_bytes,sizeof pqb_snapshot))
        return fail("PQL invalid code/access/truncation failure atomicity");
    if(port_exe_creprc_release_pqb(&pcb_allocator,&pqb_allocation)!=
          PORT_EXE_CREPRC_ALLOC_OK||pool_ipl!=0u||
       port_exe_np_pool_check(&pcb_pool)!=PORT_EXE_NP_POOL_OK)
        return fail("PQB allocation rollback returns/coalesces NP pool");
    if(port_exe_creprc_release_pcb(&pcb_allocator,&pcb_allocation)!=
          PORT_EXE_CREPRC_ALLOC_OK||pool_ipl!=0u||
       port_exe_np_pool_check(&pcb_pool)!=PORT_EXE_NP_POOL_OK)
        return fail("PCB allocation rollback returns/coalesces NP pool");
    _Alignas(8) uint8_t small_pool_memory[152];
    struct port_exe_np_pool small_pool={0};
    struct port_exe_np_allocation untouched={
        (void *)(uintptr_t)0x1000u,160u,PORT_EXE_NP_FROM_VARIABLE,
        PORT_V37_DYN_PCB};
    const struct port_exe_np_allocation untouched_before=untouched;
    if(port_exe_np_pool_init(&small_pool,small_pool_memory,
          sizeof small_pool_memory,&pool_sync)!=PORT_EXE_NP_POOL_OK)
        return fail("small NP pool fixture");
    pcb_allocator.variable=&small_pool;
    if(port_exe_creprc_allocate_pcb(&pcb_allocator,&untouched)!=
          PORT_EXE_CREPRC_ALLOC_NOMEM||
       memcmp(&untouched,&untouched_before,sizeof untouched))
        return fail("PCB allocation exhaustion leaves output untouched");
    uint8_t vector_storage[64];
    struct port_ini_procvec vector={0};
    const struct port_ini_proc_settings settings={
        .max_processes=4u,.system_pcb_address=0x2000u,
        .null_pcb_address=0x1000u,.dyn_pcbvec_type=0x1234u};
    if(port_ini_procvec_init(vector_storage,sizeof vector_storage,&settings,
            &vector)!=PORT_INI_PROC_OK||
       port_ini_procvec_set_pcb(&vector,1u,0x3000u)!=PORT_INI_PROC_OK)
        return fail("source PCB/sequence vector fixture");
    uint16_t process_count=2u;uint32_t pid=0u;
    if(port_exe_creprc_activate_slot(&vector,0x1000u,0x4000u,
          &process_count,4u,&pid)!=PORT_EXE_CREPRC_SLOT_OK||
       pid!=0x00010003u||process_count!=3u||
       port_ini_procvec_ptr(&vector,3u)!=0x4000u||
       port_ini_procvec_seq(&vector,3u)!=1u)
        return fail("SYSCREPRC descending free PIX, sequence and PID");
    if(port_exe_creprc_activate_slot(&vector,0x1000u,0x5000u,
          &process_count,4u,&pid)!=PORT_EXE_CREPRC_SLOT_OK||
       pid!=0x00010002u||process_count!=4u||
       port_ini_procvec_ptr(&vector,2u)!=0x5000u)
        return fail("SYSCREPRC next free PIX and process count");
    uint8_t *const slot2=vector.pcb_vector+8u;
    uint8_t *const seq2=vector.sequence_vector+4u;
    slot2[0]=0x00u;slot2[1]=0x10u;slot2[2]=0u;slot2[3]=0u;
    seq2[0]=0xffu;seq2[1]=0x7fu;pid=0xdeadbeefu;
    if(port_exe_creprc_activate_slot(&vector,0x1000u,0x6000u,
          &process_count,4u,&pid)!=PORT_EXE_CREPRC_SLOT_EXHAUSTED||
       pid!=0xdeadbeefu||port_ini_procvec_seq(&vector,2u)!=0x7fffu||
       port_ini_procvec_ptr(&vector,2u)!=0x1000u||process_count!=4u)
        return fail("process limit refusal leaves vector/sequence/PID unchanged");
    --process_count;
    if(port_exe_creprc_activate_slot(&vector,0x1000u,0x6000u,
          &process_count,4u,&pid)!=PORT_EXE_CREPRC_SLOT_OK||
       pid!=2u||port_ini_procvec_seq(&vector,2u)!=0u||process_count!=4u)
        return fail("PID sequence wraps at signed-positive boundary");
    slot2[0]=0x00u;slot2[1]=0x60u;slot2[2]=0u;slot2[3]=0u;
    if(port_exe_creprc_activate_slot(&vector,0x1000u,0x7000u,
          &process_count,5u,&pid)!=PORT_EXE_CREPRC_SLOT_EXHAUSTED||
       port_ini_procvec_ptr(&vector,0u)!=0x1000u)
        return fail("PIX zero remains reserved for NULL; no free process slot");
    struct port_ini_procvec malformed_vector=vector;
    malformed_vector.system_pix++;
    if(port_exe_creprc_activate_slot(&malformed_vector,0x1000u,0x8000u,
          &process_count,5u,&pid)!=PORT_EXE_CREPRC_SLOT_BAD)
        return fail("reject malformed source PIX/system/slot geometry");
    puts("SYSCREPRC preflight, PCB/PQB allocation, PIX/PID activation semantics PASS");
    return 0;
}
