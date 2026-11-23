/* Source: RTL/SRC/MTHDLOG.MAR, pinned DEC source revision.
 * Original primary .ENTRY at line 349 of 589.
 * Native IA-32 numerical adapter. Original VAX polynomial rounding,
 * condition dispatcher, register/PSL ABI, and remaining entry points
 * remain unresolved. APPROX means a native x87 approximation, not exact DEC rounding.
 * No original restricted source is distributed. */
#include "../vms_rtl_m109.h"
int port109_mthdlog(const uint16_t *arg,uint16_t *out){
    return port109_unary(PORT_M107_D,PORT109_LOG,arg,out);
}

int port109_mthdlog10(const uint16_t *arg,uint16_t *out){
    return port109_unary(PORT_M107_D,PORT109_LOG10,arg,out);
}

int port109_mthdlog2(const uint16_t *arg,uint16_t *out){
    return port109_unary(PORT_M107_D,PORT109_LOG2,arg,out);
}
