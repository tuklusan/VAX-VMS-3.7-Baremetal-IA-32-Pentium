#include "vms_con_port.h"
/* CONINTDSP.MAR 527-550: release Ctrl-S and STOP/STOP2; timer depends
 * on source burst length /4, with +2 tick minimum, wrapping at 32 bits. */
uint32_t port_v37_con_resume(uint8_t *u,size_t n,uint32_t now,
                              const struct port_v37_con_backend *b){
    if(port_v37_con_valid(u,n)!=PORT_V37_CON_OK)return PORT_V37_CON_BAD;
    port_v37_con_set32(u,PORT_V37_CON_STATE,
        port_v37_con_get32(u,PORT_V37_CON_STATE)&~(uint32_t)PORT_V37_CON_CTRLS);
    uint16_t hold=(uint16_t)(port_v37_con_get16(u,PORT_V37_CON_HOLD)&
                       (uint16_t)~(PORT_V37_CON_STOP|PORT_V37_CON_STOP2));
    port_v37_con_set16(u,PORT_V37_CON_HOLD,hold);
    const uint32_t delta=2u+((hold&PORT_V37_CON_BURST)?
        (uint32_t)(port_v37_con_get16(u,PORT_V37_CON_OUTLEN)/4u):0u);
    port_v37_con_set32(u,PORT_V37_CON_DUETIM,now+delta);
    port_v37_con_set16(u,PORT_V37_CON_STS,
        (uint16_t)(port_v37_con_get16(u,PORT_V37_CON_STS)|PORT_V37_CON_TIM));
    return port_v37_con_kick(u,b);
}
