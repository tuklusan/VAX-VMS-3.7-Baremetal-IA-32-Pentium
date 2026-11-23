/* Source: RTL/SRC/MTHGACOS.MAR, pinned DEC source revision.
 * Original primary .ENTRY at line 164 of 410.
 * Native IA-32 numerical adapter. Original VAX polynomial rounding,
 * condition dispatcher, register/PSL ABI, and remaining entry points
 * remain unresolved. APPROX means a native x87 approximation, not exact DEC rounding.
 * No original restricted source is distributed. */
#include "../vms_rtl_m109.h"
int port109_mthgacos(const uint16_t *arg,uint16_t *out){
    return port109_unary(PORT_M107_G,PORT109_ACOS,arg,out);
}

int port109_mthgacosd(const uint16_t *arg,uint16_t *out){
    return port109_unary_degrees(PORT_M107_G,PORT109_ACOS,arg,out);
}
