/* Original: RTL/SRC/LIBTRAA2E.MAR pinned source revision; first entry at line 167.
 * Freestanding native IA-32 functional subset; VAX call frames, polynomial
 * rounding, errors and guest link are NOT completed. No DEC source copied. */
#include "../vms_rtl_m110.h"
int port110_libtraa2e(const uint8_t *in,unsigned n,uint8_t *out,unsigned max){return port110_asc_to_ebc(in,n,out,max);}
