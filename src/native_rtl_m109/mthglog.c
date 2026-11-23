/* Source: RTL/SRC/MTHGLOG.MAR, pinned DEC source revision.
 * Original primary .ENTRY at line 323 of 574.
 * Native IA-32 numerical adapter. Original VAX polynomial rounding,
 * condition dispatcher, register/PSL ABI, and remaining entry points
 * remain unresolved. APPROX means a native x87 approximation, not exact DEC rounding.
 * No original restricted source is distributed. */
#include "../vms_rtl_m109.h"
int port109_mthglog(const uint16_t *arg,uint16_t *out){
    return port109_unary(PORT_M107_G,PORT109_LOG,arg,out);
}

int port109_mthglog10(const uint16_t *arg,uint16_t *out){
    return port109_unary(PORT_M107_G,PORT109_LOG10,arg,out);
}

int port109_mthglog2(const uint16_t *arg,uint16_t *out){
    return port109_unary(PORT_M107_G,PORT109_LOG2,arg,out);
}
