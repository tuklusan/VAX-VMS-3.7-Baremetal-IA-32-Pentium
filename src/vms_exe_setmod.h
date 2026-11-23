#ifndef VMS_EXE_SETMOD_H
#define VMS_EXE_SETMOD_H
#include <stdint.h>
#include "vms_exe_gettim.h"
/* SYS/SRC/SYSSETMOD.MAR: process mode bits used by three services. */
struct port_exe_process_modes {
    uint8_t implicit_wait_disabled;
    uint8_t failure_exception[4];
    uint8_t swapping_enabled;
    uint8_t previous_mode;
    uint8_t pswapm_privilege;
};
void port_exe_modes_init(struct port_exe_process_modes *m,uint8_t previous_mode);
uint32_t port_exe_setrwm(struct port_exe_process_modes *m,uint32_t wait_flag);
uint32_t port_exe_setsfm(struct port_exe_process_modes *m,uint32_t enable_flag);
uint32_t port_exe_setswm(struct port_exe_process_modes *m,uint32_t swap_flag);
#endif
