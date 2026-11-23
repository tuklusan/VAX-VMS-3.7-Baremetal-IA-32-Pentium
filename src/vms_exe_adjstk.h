#ifndef VMS_EXE_ADJSTK_H
#define VMS_EXE_ADJSTK_H
#include <stdint.h>
#include "vms_exe_access.h"
#include "vms_exe_gettim.h"
/* SYS/SRC/SYSADJSTK.MAR: outer-mode saved stack pointers. */
struct port_exe_outer_stacks {
    uint32_t mode_sp[4];
    uint8_t previous_mode;
};
struct port_exe_adjust_stack_ops {
    const struct port_exe_access *user;
    void *context;
    int (*probe_growth)(void *,uint8_t,uint32_t,uint32_t);
    uint32_t (*expand_user_stack)(void *,uint32_t);
};
uint32_t port_exe_adjstk(struct port_exe_outer_stacks *stack,uint32_t mode,
                         uint32_t adjust,uint32_t result_address,
                         const struct port_exe_adjust_stack_ops *ops);
#endif
