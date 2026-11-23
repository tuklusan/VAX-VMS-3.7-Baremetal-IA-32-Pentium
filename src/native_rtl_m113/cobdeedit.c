/* Native IA-32 source-specific partial entry; original: RTL/SRC/COBDEEDIT.MAR, line 94.
 * No DEC source is bundled. VAX calling mask, raised conditions, RTL integration not yet reproduced. */
#include "../vms_rtl_m113.h"
int port113_src_cobdeedit(uint8_t c,uint8_t *out){return port113_table(6,c,out);}
