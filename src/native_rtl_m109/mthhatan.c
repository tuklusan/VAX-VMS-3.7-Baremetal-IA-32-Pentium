/* Source: RTL/SRC/MTHHATAN.MAR, pinned DEC source revision.
 * Original primary .ENTRY at line 445 of 1077.
 * Native IA-32 numerical adapter. Original VAX polynomial rounding,
 * condition dispatcher, register/PSL ABI, and remaining entry points
 * remain unresolved. APPROX means a native x87 approximation, not exact DEC rounding.
 * No original restricted source is distributed. */
#include "../vms_rtl_m109.h"
int port109_mthhatan(const uint16_t *arg,uint16_t *out){
    return port109_unary(PORT_M107_H,PORT109_ATAN,arg,out);
}

int port109_mthhatand(const uint16_t *arg,uint16_t *out){
    return port109_unary_degrees(PORT_M107_H,PORT109_ATAN,arg,out);
}

int port109_mthhatan2(const uint16_t *num,const uint16_t *den,uint16_t *out){
    return port109_atan_ratio(PORT_M107_H,num,den,0,out);
}
int port109_mthhatand2(const uint16_t *num,const uint16_t *den,uint16_t *out){
    return port109_atan_ratio(PORT_M107_H,num,den,1,out);
}
