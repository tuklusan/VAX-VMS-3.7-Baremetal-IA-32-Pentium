/* Original: RTL/SRC/MTHCVTDG.MAR pinned source revision; first entry at line 174.
 * Freestanding native IA-32 functional subset; VAX call frames, polynomial
 * rounding, errors and guest link are NOT completed. No DEC source copied. */
#include "../vms_rtl_m110.h"
int port110_mthcvtdg_d_g(const uint16_t *a,uint16_t *out){return port110_convert(PORT_M107_D,PORT_M107_G,a,out);}
int port110_mthcvtdg_g_d(const uint16_t *a,uint16_t *out){return port110_convert(PORT_M107_G,PORT_M107_D,a,out);}
