#include "vms_sysqio_args.h"
#include "vms_exe_schevt.h"
#include "vms_sys85.h"

/* SYSQIOREQ.MAR ACCESS checks ASTCNT only for requests carrying an AST. The
 * charge itself occurs while constructing the already allocated IRP. */
uint32_t port_v37_sysqio_check_ast_quota(
    const struct port_v37_sysqio_prepared *request,
    const struct port_v37_sysqio_process *process,uint32_t *service_status){
    if(!request||!process||!service_status||!process->pid)
        return PORT_V37_SYSQIO_AST_BAD;
    if(request->args.ast_address&&
       port_v37_sysqio_process_ast_count(process)<0){
        *service_status=PORT_SS_EXQUOTA;
        return PORT_V37_SYSQIO_AST_EXQUOTA;
    }
    *service_status=PORT_SYS85_NORMAL;
    return PORT_V37_SYSQIO_AST_OK;
}
