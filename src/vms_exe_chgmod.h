#ifndef VMS_EXE_CHGMOD_H
#define VMS_EXE_CHGMOD_H
#include <stdint.h>
#include "vms_exe_gettim.h"
/* SYS/SRC/SYSCHGMOD.MAR: CALLG in privileged processor modes. */
struct port_exe_mode_context {
    uint8_t previous_mode,has_cmexec,has_cmkrnl;
};
struct port_exe_mode_executor {
    void *context;
    /* Execute under the native CPU protection and stack of target mode. */
    uint32_t (*call_mode)(void *,uint8_t,uint32_t,uint32_t);
};
uint32_t port_exe_cmexec(const struct port_exe_mode_context *process,
                         uint32_t routine,uint32_t arglist,
                         const struct port_exe_mode_executor *ops);
uint32_t port_exe_cmkrnl(const struct port_exe_mode_context *process,
                         uint32_t routine,uint32_t arglist,
                         const struct port_exe_mode_executor *ops);
#endif
