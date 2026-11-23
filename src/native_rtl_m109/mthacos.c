/* Source: RTL/SRC/MTHACOS.MAR, pinned DEC source revision.
 * Original primary .ENTRY at line 169 of 411.
 * Native IA-32 numerical adapter. Original VAX polynomial rounding,
 * condition dispatcher, register/PSL ABI, and remaining entry points
 * remain unresolved. APPROX means a native x87 approximation, not exact DEC rounding.
 * No original restricted source is distributed. */
#include "../vms_rtl_m109.h"
int port109_mthacos(const uint16_t *arg,uint16_t *out){
    return port109_unary(PORT_M107_F,PORT109_ACOS,arg,out);
}

int port109_mthacosd(const uint16_t *arg,uint16_t *out){
    return port109_unary_degrees(PORT_M107_F,PORT109_ACOS,arg,out);
}
