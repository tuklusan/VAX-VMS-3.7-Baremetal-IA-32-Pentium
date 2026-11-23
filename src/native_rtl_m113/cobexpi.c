/* Native IA-32 source-specific partial entry; original: RTL/SRC/COBEXPI.MAR, line 397; line 400.
 * No DEC source is bundled. VAX calling mask, raised conditions, RTL integration not yet reproduced. */
#include "../vms_rtl_m113.h"
int port113_src_expi_ose(int64_t base,int32_t exponent,int64_t *out){return port113_cobexpi(base,exponent,out);}
int port113_src_expi(int64_t base,int32_t exponent,int64_t *out){return port113_cobexpi(base,exponent,out);}
