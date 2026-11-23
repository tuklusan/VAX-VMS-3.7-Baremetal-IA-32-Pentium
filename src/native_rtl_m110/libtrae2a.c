/* Original: RTL/SRC/LIBTRAE2A.MAR pinned source revision; first entry at line 170.
 * Freestanding native IA-32 functional subset; VAX call frames, polynomial
 * rounding, errors and guest link are NOT completed. No DEC source copied. */
#include "../vms_rtl_m110.h"
int port110_libtrae2a(const uint8_t *in,unsigned n,uint8_t *out,unsigned max){return port110_ebc_to_asc(in,n,out,max);}
