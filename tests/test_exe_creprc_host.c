#include "vms_exe_creprc.h"
#include "vms_exe_gettim.h"
#include "vms_exe_setprv.h"
#include <stdio.h>
#include <string.h>

static int fail(const char *name){fprintf(stderr,"CREPRC FAIL: %s\n",name);return 1;}
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
    puts("SYSCREPRC policy and source PIX/PID activation semantics PASS");
    return 0;
}
