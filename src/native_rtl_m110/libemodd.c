/* Original: RTL/SRC/LIBEMODD.MAR pinned source revision; first entry at line 153.
 * Freestanding native IA-32 functional subset; VAX call frames, polynomial
 * rounding, errors and guest link are NOT completed. No DEC source copied. */
#include "../vms_rtl_m110.h"
int port110_libemodd(const uint16_t *r,uint16_t ext,const uint16_t *m,int32_t *ip,uint16_t *fp){return port110_emod(PORT_M107_D,r,ext,m,ip,fp);}
