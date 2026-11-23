/* Original: RTL/SRC/LIBPOLYD.MAR pinned source revision; first entry at line 155.
 * Freestanding native IA-32 functional subset; VAX call frames, polynomial
 * rounding, errors and guest link are NOT completed. No DEC source copied. */
#include "../vms_rtl_m110.h"
int port110_libpolyd(const uint16_t *x,unsigned order,const uint16_t *coeff,uint16_t *out){return port110_poly(PORT_M107_D,x,order,coeff,out);}
