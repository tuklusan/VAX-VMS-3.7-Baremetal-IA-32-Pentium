/* Native IA-32 source-specific partial entry; original: RTL/SRC/LIBCVTDF.MAR, line 135.
 * No DEC source is bundled. VAX calling mask, raised conditions, RTL integration not yet reproduced. */
#include "../vms_rtl_m113.h"
int port113_src_libcvtdf(const uint16_t *d,uint16_t *f){long double x; if(!d||!f)return PORT113_BAD_ARG; if(port109_decode(PORT_M107_D,d,&x)!=PORT109_OK)return PORT113_BAD_ARG; return port109_encode(PORT_M107_F,x,f)==PORT109_OK?PORT113_APPROX:PORT113_OVERFLOW;}
