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
    puts("SYSCREPRC UIC/status/priority/privilege preflight semantics PASS");
    return 0;
}
