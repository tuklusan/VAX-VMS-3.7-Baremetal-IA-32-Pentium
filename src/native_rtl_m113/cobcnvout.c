/* Native IA-32 source-specific partial entry; original: RTL/SRC/COBCNVOUT.MAR, line 163.
 * No DEC source is bundled. VAX calling mask, raised conditions, RTL integration not yet reproduced. */
#include "../vms_rtl_m113.h"
int port113_src_cobcnvout(int64_t value,char *dst,size_t length){return port113_cobcnvout(value,dst,length,1);}
