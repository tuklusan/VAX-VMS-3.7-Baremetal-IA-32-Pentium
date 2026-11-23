#include "vms_ini_devicedb.h"
/* DEVICEDAT.MAR 209-227: 132-byte UCB plus 30 longwords of boot
 * disk extension. Nonportable CSR/CRB/DDT/device class await live driver. */
void port_v37_ucb_boot(uint8_t *u,uint32_t addr,uint32_t ddb){
    port_v37_ucb_base(u,addr,ddb,PORT_V37_BOOT_UCB_SIZE);
    u[11u]=8u;u[82u]=21u;
    port_v37_write16(u+58u,512u);
    u[112u]=8u;u[113u]=8u;
}
