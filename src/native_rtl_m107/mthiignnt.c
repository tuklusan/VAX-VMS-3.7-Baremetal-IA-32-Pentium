/* Original: RTL/SRC/MTHIIGNNT.MAR (read-only DEC reference).
 * Native IA-32 operation: The VAX CVTR result keeps its low result bits on integer overflow; callers receive status.
 * Not a complete VAX calling-convention port and not a guest-loaded RTL.
 * No original source text is distributed here.
 */
#include "../vms_rtl_m107.h"
int port_m107_mthiignnt(const uint16_t *a,int32_t *r){
    return port_m107_float_rounded(PORT_M107_G,a,16u,r);
}
