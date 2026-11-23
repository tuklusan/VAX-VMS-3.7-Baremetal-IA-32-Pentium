/* Native IA-32 source-specific partial entry; original: RTL/SRC/LIBADDX.MAR, line 163; line 281.
 * No DEC source is bundled. VAX calling mask, raised conditions, RTL integration not yet reproduced. */
#include "../vms_rtl_m113.h"
int port113_src_addx(const uint32_t *a,const uint32_t *b,uint32_t *out,size_t n){return port113_addx(a,b,out,n,0);}
int port113_src_subx(const uint32_t *a,const uint32_t *b,uint32_t *out,size_t n){return port113_addx(a,b,out,n,1);}
