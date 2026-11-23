/* Original: RTL/SRC/STRMOVQ.MAR at pinned DEC reference.
 * MOVQ copies all eight bytes with overlap-safe scratch.
 * Host-tested IA-32 subset, NOT a complete translated VMS RTL image.
 * No original DEC source text is included.
 */
#include "../vms_rtl_m108.h"
int port108_strmovq(const uint8_t *src,uint8_t *dst){return port108_movq(src,dst);}
