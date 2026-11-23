#include "vms_ini_fin_database.h"
/* Fixed LOG$C_LENGTH=20 name SYS$DISK has an 8-character ASCIC string,
 * then the destination ASCIC length at +29 and data from +30. */
void port_ini_fin_disk_record(uint8_t *disk,const char *physical,unsigned n){
    disk[PORT_FINDB_DISK_EQ_OFF]=(uint8_t)n;
    for(unsigned i=PORT_FINDB_DISK_START;i<PORT_INI_LOG_TEMPLATE_BYTES;i++)
        disk[i]=(i<PORT_FINDB_DISK_START+n)?(uint8_t)physical[i-PORT_FINDB_DISK_START]:0u;
}
