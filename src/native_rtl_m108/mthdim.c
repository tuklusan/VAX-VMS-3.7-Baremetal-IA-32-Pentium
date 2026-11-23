/* Original: RTL/SRC/MTHDIM.MAR at pinned DEC reference.
 * IIDIM/JIDIM exact wrapped subtraction; F/D/G/H DIM only exact integral bounded subset.
 * Host-tested IA-32 subset, NOT a complete translated VMS RTL image.
 * No original DEC source text is included.
 */
#include "../vms_rtl_m108.h"
int port108_mthdim_i2(int16_t a,int16_t b,int16_t *out){return port108_integer_dim16(a,b,out);}
int port108_mthdim_i4(int32_t a,int32_t b,int32_t *out){return port108_integer_dim32(a,b,out);}
int port108_mthdim_float(enum port_m107_fmt f,const uint16_t *a,const uint16_t *b,uint16_t *out){return port108_float_dim(f,a,b,out);}
