#include "vms_exe_setmod.h"
#include <stddef.h>
/* SETMOD returns the old state, and then writes the low bit of the input. */
static uint32_t set_mode(uint8_t *bit,uint32_t flag) {
    uint32_t old=*bit?9u:PORT_SS_NORMAL;
    *bit=(uint8_t)(flag&1u);
    return old;
}
void port_exe_modes_init(struct port_exe_process_modes *m,uint8_t previous_mode) {
    if(!m)return;
    m->implicit_wait_disabled=0;
    for(unsigned i=0;i<4u;i++)m->failure_exception[i]=0;
    m->swapping_enabled=1;
    m->previous_mode=previous_mode&3u;
    m->pswapm_privilege=0;
}
uint32_t port_exe_setrwm(struct port_exe_process_modes *m,uint32_t flag) {
    if(!m)return PORT_SS_BADPARAM;
    return set_mode(&m->implicit_wait_disabled,flag);
}
uint32_t port_exe_setsfm(struct port_exe_process_modes *m,uint32_t flag) {
    if(!m)return PORT_SS_BADPARAM;
    return set_mode(&m->failure_exception[m->previous_mode&3u],flag);
}
uint32_t port_exe_setswm(struct port_exe_process_modes *m,uint32_t flag) {
    if(!m)return PORT_SS_BADPARAM;
    if(!m->pswapm_privilege)return PORT_SS_NOPRIV;
    return set_mode(&m->swapping_enabled,flag);
}
