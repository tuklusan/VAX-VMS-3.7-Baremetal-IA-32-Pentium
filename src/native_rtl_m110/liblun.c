/* Original: RTL/SRC/LIBLUN.MAR pinned source revision; first entry at line 150.
 * Freestanding native IA-32 functional subset; VAX call frames, polynomial
 * rounding, errors and guest link are NOT completed. No DEC source copied. */
#include "../vms_rtl_m110.h"
int port110_liblun_get(struct port110_lun_pool *p,unsigned *n){return port110_lun_get(p,n);}
int port110_liblun_free(struct port110_lun_pool *p,unsigned n){return port110_lun_free(p,n);}
