/* Original: RTL/SRC/LIBEF.MAR pinned source revision; first entry at line 145.
 * Freestanding native IA-32 functional subset; VAX call frames, polynomial
 * rounding, errors and guest link are NOT completed. No DEC source copied. */
#include "../vms_rtl_m110.h"
int port110_libef_get(struct port110_ef_pool *p,unsigned *n){return port110_ef_get(p,n);}
int port110_libef_free(struct port110_ef_pool *p,unsigned n){return port110_ef_free(p,n);}
int port110_libef_reserve(struct port110_ef_pool *p,unsigned n){return port110_ef_reserve(p,n);}
