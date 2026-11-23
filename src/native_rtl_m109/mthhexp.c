/* Source: RTL/SRC/MTHHEXP.MAR, pinned DEC source revision.
 * Original primary .ENTRY at line 312 of 466.
 * Native IA-32 numerical adapter. Original VAX polynomial rounding,
 * condition dispatcher, register/PSL ABI, and remaining entry points
 * remain unresolved. APPROX means a native x87 approximation, not exact DEC rounding.
 * No original restricted source is distributed. */
#include "../vms_rtl_m109.h"
int port109_mthhexp(const uint16_t *arg,uint16_t *out){
    return port109_unary(PORT_M107_H,PORT109_EXP,arg,out);
}
