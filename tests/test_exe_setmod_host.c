#include "vms_exe_setmod.h"
#include <assert.h>
#include <stdio.h>
int main(void) {
    struct port_exe_process_modes m;
    port_exe_modes_init(&m,3u);
    assert(port_exe_setrwm(&m,1u)==1u);
    assert(m.implicit_wait_disabled==1u);
    assert(port_exe_setrwm(&m,2u)==9u);
    assert(m.implicit_wait_disabled==0u);
    assert(port_exe_setsfm(&m,1u)==1u);
    assert(m.failure_exception[3]==1u && m.failure_exception[0]==0u);
    m.previous_mode=1;
    assert(port_exe_setsfm(&m,1u)==1u);
    assert(port_exe_setsfm(&m,3u)==9u);
    assert(port_exe_setswm(&m,0u)==36u);
    assert(m.swapping_enabled==1);
    m.pswapm_privilege=1;
    assert(port_exe_setswm(&m,0u)==9u && m.swapping_enabled==0u);
    assert(port_exe_setswm(&m,1u)==1u && m.swapping_enabled==1u);
    assert(port_exe_setswm(&m,1u)==9u);
    assert(port_exe_setrwm(0,1u)==20u);
    puts("SYSSETMOD source services pass");
    return 0;
}
