/* Original: RTL/SRC/MTHCDSQRT.MAR pinned source revision; first entry at line 134.
 * Freestanding native IA-32 functional subset; VAX call frames, polynomial
 * rounding, errors and guest link are NOT completed. No DEC source copied. */
#include "../vms_rtl_m110.h"
int port110_mthcdsqrt(const uint16_t *a,uint16_t *out){return port110_complex(PORT_M107_D,PORT110_CSQRT,a,out);}
