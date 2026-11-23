/* Native IA-32 source-specific partial entry; original: RTL/SRC/LIBASTINP.MAR, line 125.
 * No DEC source is bundled. VAX calling mask, raised conditions, RTL integration not yet reproduced. */
#include "../vms_rtl_m113.h"
int port113_src_libastinp(uint32_t mode,unsigned in_ast,int *out){return port113_libastinp(mode,in_ast,out);}
