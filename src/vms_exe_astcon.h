#ifndef VMS_EXE_ASTCON_H
#define VMS_EXE_ASTCON_H
#include <stdint.h>
#include "vms_exe_gettim.h"
#include "vms_exe_schevt.h"
/* SYS/SRC/SYSASTCON.MAR: AST enable state, quota and queue request. */
struct port_exe_astcon_process {
    uint32_t pid;
    uint16_t ast_available;
    uint8_t previous_mode, ast_enabled;
};
struct port_exe_astcon_ops {
    void *context;
    void (*new_ast_level)(void *,struct port_exe_astcon_process *);
    /* The allocator returns a real ACB handle or a failure status. */
    uint32_t (*allocate)(void *,void **);
    uint32_t (*enqueue)(void *,void *,uint32_t,uint32_t,uint32_t,uint8_t);
    void (*release)(void *,void *);
};
uint32_t port_exe_setast(struct port_exe_astcon_process *process,
                         uint32_t enable,const struct port_exe_astcon_ops *ops);
uint32_t port_exe_dclast(struct port_exe_astcon_process *process,
                         uint32_t handler,uint32_t parameter,uint32_t mode,
                         const struct port_exe_astcon_ops *ops);
#endif
