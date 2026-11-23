/* Original: RTL/SRC/MTHSGN.MAR (read-only DEC reference).
 * Native IA-32 operation: Returns -1, 0 or +1 for VAX F signs; the R1 assembly service alias remains unresolved.
 * Not a complete VAX calling-convention port and not a guest-loaded RTL.
 * No original source text is distributed here.
 */
#include "../vms_rtl_m107.h"
int port_m107_mthsgn(const uint16_t *v,int *result){
    static const uint16_t zero[2]={0,0};
    if(!result)return PORT_M107_BAD_ARG;
    return port_m107_float_cmp(PORT_M107_F,v,zero,result);
}
