/* Original: RTL/SRC/MTHMOD.MAR at pinned DEC reference.
 * IMOD/JMOD signed remainder; divide-by-zero is an explicit status, not a VAX condition.
 * Host-tested IA-32 subset, NOT a complete translated VMS RTL image.
 * No original DEC source text is included.
 */
#include "../vms_rtl_m108.h"
int port108_mthmod_i2(int16_t a,int16_t b,int16_t *out){return port108_integer_mod16(a,b,out);}
int port108_mthmod_i4(int32_t a,int32_t b,int32_t *out){return port108_integer_mod32(a,b,out);}
