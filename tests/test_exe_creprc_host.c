#include "vms_exe_creprc.h"
#include "vms_exe_gettim.h"
#include "vms_exe_setprv.h"
#include "vms_v37_dyn_tags.h"
#include "vms_v37_pcb.h"
#include <stdio.h>
#include <string.h>

static int fail(const char *name){fprintf(stderr,"CREPRC FAIL: %s\n",name);return 1;}
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
    _Alignas(8) uint8_t pcb_pool_memory[512];
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
    puts("SYSCREPRC preflight, PCB allocation, PIX/PID activation semantics PASS");
    return 0;
}
