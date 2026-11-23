#include "vms_ini_devicedb.h"
/* DEVICEDAT.MAR 396-420: permanent SYS$GL_JOBCTLMB, MBA1. */
void port_v37_ucb_mb1(uint8_t *u,uint32_t addr,uint32_t ddb,uint32_t next){
    port_v37_ucb_base(u,addr,ddb,PORT_V37_UCB_SIZE);
    port_v37_write32(u+0u,addr);port_v37_write32(u+4u,addr);
    port_v37_write16(u+20u,60u);
    port_v37_write16(u+24u,0xffffu);port_v37_write16(u+26u,0xff0fu);
    port_v37_write32(u+28u,0x00010004u);
    port_v37_write32(u+44u,next);
    port_v37_write16(u+58u,1024u);
    port_v37_write16(u+72u,1u);port_v37_write16(u+80u,1u);
    port_v37_write16(u+88u,PORT_V37_UCB_ONLINE);
    port_v37_write16(u+90u,0x8001u);
    /* original IPL$_MAILBOX, class code, CRB and DDT unresolved */
}
