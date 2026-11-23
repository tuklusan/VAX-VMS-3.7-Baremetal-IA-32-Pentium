/* Original: RTL/SRC/MTHHMOD.MAR at pinned DEC reference.
 * Exact VAX H integer-valued subset only; fractional EMOD and signaling still unresolved.
 * Host-tested IA-32 subset, NOT a complete translated VMS RTL image.
 * No original DEC source text is included.
 */
#include "../vms_rtl_m108.h"
int port108_mthhmod(const uint16_t *a,const uint16_t *b,uint16_t *out){
    return port108_float_mod(PORT_M107_H,a,b,out);
}
