#include "vms_ini_devicedb.h"
/* DEVICEDAT.MAR 460-478: NULL NLA0 unit and source-sized UCB. */
void port_v37_ucb_nl(uint8_t *u,uint32_t addr,uint32_t ddb){
    port_v37_ucb_base(u,addr,ddb,PORT_V37_UCB_SIZE);
    u[11u]=8u;u[82u]=8u;
    port_v37_write16(u+58u,132u);
    port_v37_write16(u+80u,1u);
    port_v37_write16(u+88u,PORT_V37_UCB_ONLINE);
    /* Original SYS_CRB and NL$DDT not yet usable; keep zero. */
}
