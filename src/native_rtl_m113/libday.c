/* Native IA-32 source-specific partial entry; original: RTL/SRC/LIBDAY.MAR, line 155.
 * No DEC source is bundled. VAX calling mask, raised conditions, RTL integration not yet reproduced. */
#include "../vms_rtl_m113.h"
int port113_src_libday(uint64_t vax_quadword_time,int32_t *day,uint32_t *centis){return port113_libday(vax_quadword_time,day,centis);}
