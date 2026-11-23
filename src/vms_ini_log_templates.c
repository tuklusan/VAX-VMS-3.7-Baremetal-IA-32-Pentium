#include "vms_ini_log_templates.h"
/* Re-creates structure values, not original DEC assembly text. The source
 * counts are ASCIC (length byte followed by literal text), not C strings. */
static void word(uint8_t *p,uint16_t x){p[0]=(uint8_t)x;p[1]=(uint8_t)(x>>8);}
static void clear(uint8_t *p){for(unsigned i=0;i<PORT_INI_LOG_TEMPLATE_BYTES;i++)p[i]=0u;}
static void copy(uint8_t *p,const char *s,unsigned n){for(unsigned i=0;i<n;i++)p[i]=(uint8_t)s[i];}
void port_ini_log_templates(uint8_t disk[PORT_INI_LOG_TEMPLATE_BYTES],
    uint8_t sysdevice[PORT_INI_LOG_TEMPLATE_BYTES]){
    if(!disk||!sysdevice)return;
    clear(disk);clear(sysdevice);
    word(disk+8u,PORT_INI_LOG_TEMPLATE_BYTES);
    word(sysdevice+8u,PORT_INI_LOG_TEMPLATE_BYTES);
    disk[10u]=sysdevice[10u]=PORT_V37_DYN_LOG;
    /* LOG$C_SYSTEM=0 (SYSGETDEV.MAR); UIC member/group both 1. */
    word(disk+12u,1u);word(disk+14u,1u);
    word(sysdevice+12u,1u);word(sysdevice+14u,1u);
    disk[20u]=8u;copy(disk+21u,"SYS$DISK",8u);
    disk[29u]=5u;copy(disk+30u,"_DXX000:",8u);
    sysdevice[20u]=13u;copy(sysdevice+21u,"SYS$SYSDEVICE",13u);
    sysdevice[34u]=1u;copy(sysdevice+35u,"__",2u);
    copy(sysdevice+37u,"DDCU:::",7u);
}
