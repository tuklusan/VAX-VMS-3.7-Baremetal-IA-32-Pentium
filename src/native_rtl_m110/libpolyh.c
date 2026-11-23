/* Original: RTL/SRC/LIBPOLYH.MAR pinned source revision; first entry at line 152.
 * Freestanding native IA-32 functional subset; VAX call frames, polynomial
 * rounding, errors and guest link are NOT completed. No DEC source copied. */
#include "../vms_rtl_m110.h"
int port110_libpolyh(const uint16_t *x,unsigned order,const uint16_t *coeff,uint16_t *out){return port110_poly(PORT_M107_H,x,order,coeff,out);}
