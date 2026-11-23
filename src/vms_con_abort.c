#include "vms_con_port.h"
/* CONINTDSP.MAR 519-525: clear burst and schedule in-flight stop timeout. */
uint32_t port_v37_con_abort(uint8_t *u,size_t n,uint32_t now){
    if(port_v37_con_valid(u,n)!=PORT_V37_CON_OK)return PORT_V37_CON_BAD;
    port_v37_con_set16(u,PORT_V37_CON_HOLD,
        (uint16_t)(port_v37_con_get16(u,PORT_V37_CON_HOLD)&~PORT_V37_CON_BURST));
    port_v37_con_set32(u,PORT_V37_CON_DUETIM,now+2u);
    return PORT_V37_CON_OK;
}
