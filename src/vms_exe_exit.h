#ifndef VMS_EXE_EXIT_H
#define VMS_EXE_EXIT_H
#include <stdint.h>
#include "vms_exe_dclexh.h"
/* SYS/SRC/SYSEXIT.MAR: termination chains and final process status. */
struct port_exe_exit_process {
    uint32_t pid,final_status;
    uint8_t previous_mode,force_exit_pending;
    struct port_exe_exit_handlers *handlers;
};
struct port_exe_exit_ops {
    const struct port_exe_access *memory;
    void *context;
    /* Execute the handler under its original access mode with its block. */
    uint32_t (*invoke)(void *,uint8_t,uint32_t,uint32_t);
    uint32_t (*delete_self)(void *,uint32_t,uint32_t);
};
uint32_t port_exe_exit(struct port_exe_exit_process *,uint32_t,
                       const struct port_exe_exit_ops *);
#endif
