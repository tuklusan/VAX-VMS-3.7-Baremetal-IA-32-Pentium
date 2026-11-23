/* Original: RTL/SRC/MTHCGEXP.MAR pinned source revision; first entry at line 122.
 * Freestanding native IA-32 functional subset; VAX call frames, polynomial
 * rounding, errors and guest link are NOT completed. No DEC source copied. */
#include "../vms_rtl_m110.h"
int port110_mthcgexp(const uint16_t *a,uint16_t *out){return port110_complex(PORT_M107_G,PORT110_CEXP,a,out);}
