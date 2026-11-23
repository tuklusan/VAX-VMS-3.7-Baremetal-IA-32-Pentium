#include "vms_ini_fin_database.h"
/* Counted SYS$SYSDEVICE: original 13-char name + one byte count. */
void port_ini_fin_sysdev_record(uint8_t *sys,const char *value,unsigned n){
    sys[PORT_FINDB_SYSDEV_EQ_OFF]=(uint8_t)n;
    for(unsigned i=PORT_FINDB_SYSDEV_START;i<PORT_INI_LOG_TEMPLATE_BYTES;i++)
        sys[i]=(i<PORT_FINDB_SYSDEV_START+n)?(uint8_t)value[i-PORT_FINDB_SYSDEV_START]:0u;
}
