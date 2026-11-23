#ifndef VMS_EXE_SETSSF_H
#define VMS_EXE_SETSSF_H
#include <stdint.h>
#include "vms_exe_gettim.h"
/* SYS/SRC/SYSSETSSF.MAR: previous mode 0..3 is K,E,S,U. */
uint32_t port_exe_setssf(volatile uint8_t *filter,uint32_t previous_mode,
                         uint32_t mask);
#endif
