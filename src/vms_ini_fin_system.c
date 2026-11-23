#include "vms_ini_fin_database.h"
/* Original EXE$V_CONCEALED chooses one or two leading underscores for
 * SYS$SYSDEVICE, preserving the actual current boot DDB/unit name. */
uint32_t port_ini_fin_system(uint16_t unit,uint8_t concealed,
                            char out[13],uint8_t *count){
    if(!out||!count||concealed>1u)return PORT_FINDB_BAD;
    char physical[12];uint8_t n=0u;
    if(port_ini_fin_physical(unit,physical,&n)!=PORT_FINDB_OK)
        return PORT_FINDB_BAD;
    const unsigned extra=(unsigned)concealed;
    for(unsigned i=0;i<n+extra;i++)out[i]=(i<extra)?'_':physical[i-extra];
    for(unsigned i=n+extra;i<13u;i++)out[i]=0;
    *count=(uint8_t)(n+extra);
    return PORT_FINDB_OK;
}
