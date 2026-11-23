/* Source: RTL/SRC/MTHDATAN.MAR, pinned DEC source revision.
 * Original primary .ENTRY at line 422 of 984.
 * Native IA-32 numerical adapter. Original VAX polynomial rounding,
 * condition dispatcher, register/PSL ABI, and remaining entry points
 * remain unresolved. APPROX means a native x87 approximation, not exact DEC rounding.
 * No original restricted source is distributed. */
#include "../vms_rtl_m109.h"
int port109_mthdatan(const uint16_t *arg,uint16_t *out){
    return port109_unary(PORT_M107_D,PORT109_ATAN,arg,out);
}

int port109_mthdatand(const uint16_t *arg,uint16_t *out){
    return port109_unary_degrees(PORT_M107_D,PORT109_ATAN,arg,out);
}

int port109_mthdatan2(const uint16_t *num,const uint16_t *den,uint16_t *out){
    return port109_atan_ratio(PORT_M107_D,num,den,0,out);
}
int port109_mthdatand2(const uint16_t *num,const uint16_t *den,uint16_t *out){
    return port109_atan_ratio(PORT_M107_D,num,den,1,out);
}
