#ifndef VMS_EXE_FORCEX_H
#define VMS_EXE_FORCEX_H
#include <stdint.h>
#include "vms_exe_schevt.h"
/* SYS/SRC/SYSFORCEX.MAR: force-exit user AST, never a direct kill. */
struct port_exe_force_process {uint32_t pid;uint8_t force_pending;};
struct port_exe_force_ops {
    void *context;
    uint32_t (*resolve)(void *,uint32_t,uint32_t,uint32_t,
                        struct port_exe_force_process **);
    uint32_t (*queue_user_exit_ast)(void *,uint32_t,uint32_t);
    uint32_t (*deliver_exit)(void *,uint32_t,uint32_t);
};
uint32_t port_exe_forcex(const struct port_exe_force_process *,uint32_t,
                         uint32_t,uint32_t,const struct port_exe_force_ops *);
uint32_t port_exe_forcex_deliver(struct port_exe_force_process *,uint32_t,
                                 const struct port_exe_force_ops *);
#endif
