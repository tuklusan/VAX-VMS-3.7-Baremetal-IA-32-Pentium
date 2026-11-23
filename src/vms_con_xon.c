#include "vms_con_port.h"
/* CONINTDSP.MAR 488-499: schedule XON and initiate output when not busy. */
uint32_t port_v37_con_xon(uint8_t *u,size_t n,const struct port_v37_con_backend *b){
    if(port_v37_con_valid(u,n)!=PORT_V37_CON_OK)return PORT_V37_CON_BAD;
    port_v37_con_set16(u,PORT_V37_CON_HOLD,
        (uint16_t)(port_v37_con_get16(u,PORT_V37_CON_HOLD)|PORT_V37_CON_XON));
    return port_v37_con_kick(u,b);
}
