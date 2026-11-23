/* Native IA-32 source-specific partial entry; original: RTL/SRC/BASEDIT.MAR, line 168.
 * No DEC source is bundled. VAX calling mask, raised conditions, RTL integration not yet reproduced. */
#include "../vms_rtl_m113.h"
int port113_src_basedit(uint8_t *chars,size_t length,int upcase,int brackets){return port113_basedit(chars,length,upcase,brackets);}
