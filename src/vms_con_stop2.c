#include "vms_con_port.h"
/* CONINTDSP.MAR 509-517: broadcast STOP2 activates two-tick timeout. */
uint32_t port_v37_con_stop2(uint8_t *u,size_t n,uint32_t now){
    if(port_v37_con_valid(u,n)!=PORT_V37_CON_OK)return PORT_V37_CON_BAD;
    port_v37_con_set32(u,PORT_V37_CON_STATE,
        port_v37_con_get32(u,PORT_V37_CON_STATE)|PORT_V37_CON_CTRLS);
    port_v37_con_set16(u,PORT_V37_CON_HOLD,
        (uint16_t)(port_v37_con_get16(u,PORT_V37_CON_HOLD)|PORT_V37_CON_STOP2));
    port_v37_con_set16(u,PORT_V37_CON_STS,
        (uint16_t)(port_v37_con_get16(u,PORT_V37_CON_STS)|PORT_V37_CON_TIM));
    port_v37_con_set32(u,PORT_V37_CON_DUETIM,now+2u);
    return PORT_V37_CON_OK;
}
