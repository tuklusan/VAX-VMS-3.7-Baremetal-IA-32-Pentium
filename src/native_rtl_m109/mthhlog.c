/* Source: RTL/SRC/MTHHLOG.MAR, pinned DEC source revision.
 * Original primary .ENTRY at line 346 of 605.
 * Native IA-32 numerical adapter. Original VAX polynomial rounding,
 * condition dispatcher, register/PSL ABI, and remaining entry points
 * remain unresolved. APPROX means a native x87 approximation, not exact DEC rounding.
 * No original restricted source is distributed. */
#include "../vms_rtl_m109.h"
int port109_mthhlog(const uint16_t *arg,uint16_t *out){
    return port109_unary(PORT_M107_H,PORT109_LOG,arg,out);
}

int port109_mthhlog10(const uint16_t *arg,uint16_t *out){
    return port109_unary(PORT_M107_H,PORT109_LOG10,arg,out);
}

int port109_mthhlog2(const uint16_t *arg,uint16_t *out){
    return port109_unary(PORT_M107_H,PORT109_LOG2,arg,out);
}
