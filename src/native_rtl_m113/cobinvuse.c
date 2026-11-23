/* Native IA-32 source-specific partial entry; original: RTL/SRC/COBINVUSE.MAR, line 140.
 * No DEC source is bundled. VAX calling mask, raised conditions, RTL integration not yet reproduced. */
#include "../vms_rtl_m113.h"
int port113_src_cobinvuse(int (*use_proc)(void *,uint32_t),void *context,uint32_t arg,int *result){return port113_cobinvoke(use_proc,context,arg,result);}
