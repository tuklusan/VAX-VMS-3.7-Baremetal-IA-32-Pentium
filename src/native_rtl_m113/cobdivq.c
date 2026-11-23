/* Native IA-32 source-specific partial entry; original: RTL/SRC/COBDIVQ.MAR, line 154.
 * No DEC source is bundled. VAX calling mask, raised conditions, RTL integration not yet reproduced. */
#include "../vms_rtl_m113.h"
int port113_src_cobdivq(int64_t numerator,int64_t denominator,int64_t *quotient,int64_t *remainder){return port113_cobdivq(numerator,denominator,quotient,remainder);}
