#include "vms_ini_devicedb.h"
/* DEVICEDAT.MAR 423-449: permanent SYS$GL_OPRMBX, MBA2. */
void port_v37_ucb_mb2(uint8_t *u,uint32_t addr,uint32_t ddb){
    port_v37_ucb_base(u,addr,ddb,PORT_V37_UCB_SIZE);
    port_v37_write32(u+0u,addr);port_v37_write32(u+4u,addr);
    port_v37_write16(u+20u,20u);
    port_v37_write16(u+24u,0xffffu);port_v37_write16(u+26u,0xff0fu);
    port_v37_write32(u+28u,0x00010004u);
    port_v37_write16(u+58u,256u);
    port_v37_write16(u+72u,2u);port_v37_write16(u+80u,1u);
    port_v37_write16(u+88u,PORT_V37_UCB_ONLINE);
    port_v37_write16(u+90u,1u);
}
