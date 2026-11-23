#ifndef VMS_EXE_SETPRV_H
#define VMS_EXE_SETPRV_H
#include <stdint.h>
#include "vms_exe_access.h"
#include "vms_exe_gettim.h"
enum { PORT_SS_IVSTSFLG=380u, PORT_SS_NOTALLPRIV=1665u,
       PORT_PRV_SETPRV_BIT=14u };
/* SYS/SRC/SYSSETPRV.MAR: current, PHD, authorized and process masks. */
struct port_exe_privileges {
    uint64_t current,process_header,permanent,authorized,image;
    uint8_t previous_mode;
};
struct port_exe_setprv_request {
    uint32_t enable_flag,privilege_address,permanent_flag,old_address;
};
uint32_t port_exe_setprv(struct port_exe_privileges *process,
                         const struct port_exe_setprv_request *request,
                         const struct port_exe_access *user);
#endif
