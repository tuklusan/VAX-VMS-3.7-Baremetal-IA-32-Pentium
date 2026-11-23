/* Original RTL/SRC/MTHCONVER.MAR: selective IA-32 native conversion entries.
 * 27 further VAX entry points, integer-overflow PSW, and original calling
 * conventions remain incomplete. No restricted source reproduced. */
#include "../vms_rtl_m110.h"
int port110_mthconver_dfloti(int16_t v,uint16_t *out){
    return port110_int_to_float(PORT_M107_D,(int32_t)v,out);
}
int port110_mthconver_dflotj(int32_t v,uint16_t *out){
    return port110_int_to_float(PORT_M107_D,v,out);
}
int port110_mthconver_iint(const uint16_t *v,int32_t *out){
    return port110_float_to_int(PORT_M107_F,v,16u,out);
}
int port110_mthconver_jint(const uint16_t *v,int32_t *out){
    return port110_float_to_int(PORT_M107_F,v,32u,out);
}
int port110_mthconver_dble(const uint16_t *v,uint16_t *out){
    return port110_convert(PORT_M107_F,PORT_M107_D,v,out);
}
int port110_mthconver_sngl(const uint16_t *v,uint16_t *out){
    return port110_convert(PORT_M107_D,PORT_M107_F,v,out);
}
