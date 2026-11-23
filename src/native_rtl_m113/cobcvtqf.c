/* Native IA-32 source-specific partial entry; original: RTL/SRC/COBCVTQF.MAR, line 145.
 * No DEC source is bundled. VAX calling mask, raised conditions, RTL integration not yet reproduced. */
#include "../vms_rtl_m113.h"
int port113_src_cobcvtqf(const void *src,size_t digits,int32_t scale,void *dst,size_t cap){return port113_convert(PORT113_QUAD_F,src,digits,scale,dst,cap,0);}
