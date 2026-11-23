#include "vms_con_port.h"
/* CONINTDSP.MAR 501-508: block class output and schedule port STOP. */
uint32_t port_v37_con_stop(uint8_t *u,size_t n){
    if(port_v37_con_valid(u,n)!=PORT_V37_CON_OK)return PORT_V37_CON_BAD;
    port_v37_con_set32(u,PORT_V37_CON_STATE,
        port_v37_con_get32(u,PORT_V37_CON_STATE)|PORT_V37_CON_CTRLS);
    port_v37_con_set16(u,PORT_V37_CON_HOLD,
        (uint16_t)(port_v37_con_get16(u,PORT_V37_CON_HOLD)|PORT_V37_CON_STOP));
    return PORT_V37_CON_OK;
}
