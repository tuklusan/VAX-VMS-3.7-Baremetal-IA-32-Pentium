#include "vms_ini_devicedb.h"
/* DEVICEDAT.MAR 237-263; TTYDEF.MAR 245-449.
 * UCB$C_TT_LENGTH=256, EXPAND=(256-132+3+64)/4=47 longwords,
 * hence 132+188=320. Actual class/port vectors are not bound yet. */
void port_v37_ucb_opa(uint8_t *u,uint32_t addr,uint32_t ddb){
    port_v37_ucb_base(u,addr,ddb,PORT_V37_OPA_UCB_SIZE);
    u[11u]=8u;u[82u]=20u;
    port_v37_write16(u+58u,132u);
    port_v37_write16(u+88u,PORT_V37_UCB_ONLINE);
    port_v37_write16(u+240u,132u); /* UCB$W_TT_DESIZE */
    /* CRB, DDT, port vectors and character flags must be source-verified
     * and connected before an active terminal $QIO may be accepted. */
}
