/* Source: RTL/SRC/MTHDACOS.MAR, pinned DEC source revision.
 * Original primary .ENTRY at line 173 of 418.
 * Native IA-32 numerical adapter. Original VAX polynomial rounding,
 * condition dispatcher, register/PSL ABI, and remaining entry points
 * remain unresolved. APPROX means a native x87 approximation, not exact DEC rounding.
 * No original restricted source is distributed. */
#include "../vms_rtl_m109.h"
int port109_mthdacos(const uint16_t *arg,uint16_t *out){
    return port109_unary(PORT_M107_D,PORT109_ACOS,arg,out);
}

int port109_mthdacosd(const uint16_t *arg,uint16_t *out){
    return port109_unary_degrees(PORT_M107_D,PORT109_ACOS,arg,out);
}
