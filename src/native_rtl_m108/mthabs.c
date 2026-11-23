/* Original: RTL/SRC/MTHABS.MAR at pinned DEC reference.
 * IIABS, JIABS, ABS, DABS, GABS and HABS; INT_MIN wraps as MNEG.
 * Host-tested IA-32 subset, NOT a complete translated VMS RTL image.
 * No original DEC source text is included.
 */
#include "../vms_rtl_m108.h"
int port108_mthabs_i2(int16_t x,int16_t *out){return port108_integer_abs16(x,out);}
int port108_mthabs_i4(int32_t x,int32_t *out){return port108_integer_abs32(x,out);}
int port108_mthabs_f(enum port_m107_fmt f,const uint16_t *x,uint16_t *out){return port108_float_transform(f,x,out,PORT108_ABS);}
