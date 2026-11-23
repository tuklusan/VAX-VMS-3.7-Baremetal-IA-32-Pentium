#ifndef VMS_EXE_SETSTK_H
#define VMS_EXE_SETSTK_H
#include <stdint.h>
#include "vms_exe_access.h"
#include "vms_exe_gettim.h"
/* SYS/SRC/SYSSETSTK.MAR: stack address bounds for each access mode. */
struct port_exe_stack_bounds {
    uint32_t low[4],high[4];
    uint8_t previous_mode;
};
uint32_t port_exe_setstk(struct port_exe_stack_bounds *stack,
                         uint32_t input,uint32_t previous,uint32_t mode,
                         const struct port_exe_access *user);
#endif
