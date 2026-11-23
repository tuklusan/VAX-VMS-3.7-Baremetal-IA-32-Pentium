/* Original: RTL/SRC/LIBPOLYG.MAR pinned source revision; first entry at line 151.
 * Freestanding native IA-32 functional subset; VAX call frames, polynomial
 * rounding, errors and guest link are NOT completed. No DEC source copied. */
#include "../vms_rtl_m110.h"
int port110_libpolyg(const uint16_t *x,unsigned order,const uint16_t *coeff,uint16_t *out){return port110_poly(PORT_M107_G,x,order,coeff,out);}
