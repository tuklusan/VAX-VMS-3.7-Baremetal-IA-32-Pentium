#include "vms_ini_fin_database.h"
/* INIT.MAR EDIV #10 / ADDB digit loop, converted directly to IA-32 C.
 * A UCB unit is an unsigned 16-bit number; no decimal exponent tables. */
uint32_t port_ini_fin_decimal(uint16_t unit,char out[5]){
    if(!out)return 0u;
    char reverse[5];unsigned n=0u;
    do{reverse[n++]=(char)('0'+(unit%10u));unit=(uint16_t)(unit/10u);}while(unit);
    for(unsigned i=0u;i<n;i++)out[i]=reverse[n-1u-i];
    return n;
}
