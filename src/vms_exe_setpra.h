#ifndef VMS_EXE_SETPRA_H
#define VMS_EXE_SETPRA_H
#include <stdint.h>
#include "vms_exe_gettim.h"
#include "vms_exe_schevt.h"
/* SYS/SRC/SYSSETPRA.MAR: registration and two-stage power-recovery AST. */
struct port_exe_power_process {
    uint32_t pid;
    uint32_t ast_address;
    uint16_t ast_available;
    uint8_t previous_mode,ast_mode,power_pending;
};
struct port_exe_power_ops {
    void *context;
    uint32_t (*queue_special_ast)(void *,uint32_t,uint32_t);
    uint32_t (*queue_user_ast)(void *,uint32_t,uint32_t,uint32_t,uint8_t);
};
uint32_t port_exe_setpra(struct port_exe_power_process *process,
                         uint32_t ast_address,uint32_t access_mode);
/* Dispatch after a physical power failure. Time is centiseconds. */
unsigned port_exe_powerast(struct port_exe_power_process *const *processes,
                            unsigned process_slots,uint32_t *down_centiseconds,
                            const struct port_exe_power_ops *ops);
uint32_t port_exe_powerast_requeue(struct port_exe_power_process *process,
                                   uint32_t down_centiseconds,
                                   const struct port_exe_power_ops *ops);
#endif
