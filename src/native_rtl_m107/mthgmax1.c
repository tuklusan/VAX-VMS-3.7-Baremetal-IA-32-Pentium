/* Original: RTL/SRC/MTHGMAX1.MAR (read-only DEC reference).
 * Native IA-32 operation: Only the base VAX format result is implemented; additional integer-return entry variants are unresolved.
 * Not a complete VAX calling-convention port and not a guest-loaded RTL.
 * No original source text is distributed here.
 */
#include "../vms_rtl_m107.h"
int port_m107_mthgmax1(const uint16_t * const *a,size_t n,uint16_t *out){
    return port_m107_float_extreme(PORT_M107_G,a,n,out,1);
}
