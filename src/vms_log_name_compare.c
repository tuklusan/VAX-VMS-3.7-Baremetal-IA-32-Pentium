#include "vms_log_system.h"
/* Original LOG$SEARCHLOG bucket walks sorted by string length then name.
 * Return 0=less, 1=equal, 2=greater (unsigned VAX CMPC order). */
uint32_t port_log_name_compare(const uint8_t *a,size_t na,
                               const uint8_t *b,size_t nb){
    if(na<nb)return 0u;
    if(na>nb)return 2u;
    for(size_t i=0u;i<na;i++){
        if(a[i]<b[i])return 0u;
        if(a[i]>b[i])return 2u;
    }
    return 1u;
}
