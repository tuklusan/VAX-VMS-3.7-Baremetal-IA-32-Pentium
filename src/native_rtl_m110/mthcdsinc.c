/* Original: RTL/SRC/MTHCDSINC.MAR pinned source revision; first entry at line 124.
 * Freestanding native IA-32 functional subset; VAX call frames, polynomial
 * rounding, errors and guest link are NOT completed. No DEC source copied. */
#include "../vms_rtl_m110.h"
int port110_mthcdsinc_sin(const uint16_t *a,uint16_t *out){return port110_complex(PORT_M107_D,PORT110_CSIN,a,out);}
int port110_mthcdsinc_cos(const uint16_t *a,uint16_t *out){return port110_complex(PORT_M107_D,PORT110_CCOS,a,out);}
