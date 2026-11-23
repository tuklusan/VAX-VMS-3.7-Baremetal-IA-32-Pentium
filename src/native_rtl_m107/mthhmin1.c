/* Original: RTL/SRC/MTHHMIN1.MAR (read-only DEC reference).
 * Native IA-32 operation: The original HMIN source uses MOVG/CMPG and an uninitialized R1: executable intended H comparison is not proven equivalent.
 * Not a complete VAX calling-convention port and not a guest-loaded RTL.
 * No original source text is distributed here.
 */
#include "../vms_rtl_m107.h"
int port_m107_mthhmin1(const uint16_t * const *a,size_t n,uint16_t *out){
    return port_m107_float_extreme(PORT_M107_H,a,n,out,0);
}
