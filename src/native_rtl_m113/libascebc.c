/* Native IA-32 source-specific partial entry; original: RTL/SRC/LIBASCEBC.MAR, line 116.
 * No DEC source is bundled. VAX calling mask, raised conditions, RTL integration not yet reproduced. */
#include "../vms_rtl_m113.h"
int port113_src_libascebc(uint8_t c,uint8_t *out){return port113_table(2,c,out);}
