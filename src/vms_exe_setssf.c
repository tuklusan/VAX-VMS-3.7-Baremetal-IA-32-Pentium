#include "vms_exe_setssf.h"
uint32_t port_exe_setssf(volatile uint8_t *filter,uint32_t previous_mode,
                         uint32_t mask) {
    if((previous_mode&3u)==3u || !filter)return PORT_SS_NOPRIV;
    *filter=(uint8_t)mask;
    return PORT_SS_NORMAL;
}
