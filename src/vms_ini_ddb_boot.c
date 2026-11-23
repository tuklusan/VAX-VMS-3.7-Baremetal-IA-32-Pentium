#include "vms_ini_devicedb.h"
/* DEVICEDAT.MAR 206-207 SYS$GL_BOOTDDB: first IOC$GL_DEVLIST node. */
void port_v37_ddb_boot(uint8_t *d,uint32_t next,uint32_t ucb){
    port_v37_ddb_base(d,next,ucb,"",0u,"",0u);
    /* ^A/F11/ plus ACP device class 1 in top byte. Source boot DDT=0. */
    port_v37_write32(d+16u,((uint32_t)'F')|((uint32_t)'1'<<8)|
                       ((uint32_t)'1'<<16)|(1u<<24));
}
