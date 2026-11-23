#include "vms_ini_fin_database.h"
/* FIN_DATABASE writes generic device letters into _DXX000: then shifts
 * one-to-five decimal digits left, updating the counted string length. */
uint32_t port_ini_fin_physical(uint16_t unit,char out[12],uint8_t *count){
    if(!out||!count)return PORT_FINDB_BAD;
    char digits[5];const uint32_t n=port_ini_fin_decimal(unit,digits);
    if(!n||n>5u)return PORT_FINDB_BAD;
    out[0]='_';out[1]='D';out[2]='Q';out[3]='A';
    for(unsigned i=0;i<n;i++)out[4u+i]=digits[i];
    out[4u+n]=':';
    const uint8_t total=(uint8_t)(5u+n);
    for(unsigned i=total;i<12u;i++)out[i]=0;
    *count=total;
    return PORT_FINDB_OK;
}
