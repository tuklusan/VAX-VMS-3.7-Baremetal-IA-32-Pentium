/* Native IA-32 source-specific partial entry; original: RTL/SRC/LIBEBCASC.MAR, line 125.
 * No DEC source is bundled. VAX calling mask, raised conditions, RTL integration not yet reproduced. */
#include "../vms_rtl_m113.h"
int port113_src_libebcasc(uint8_t c,uint8_t *out){return port113_table(3,c,out);}
