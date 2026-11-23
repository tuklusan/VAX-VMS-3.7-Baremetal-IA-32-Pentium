/* Original: RTL/SRC/MTHCABS.MAR at pinned DEC reference.
 * Exact integer Pythagorean subdomain of complex magnitude; arbitrary SQRT remains unresolved.
 * Host-tested IA-32 subset, NOT a complete translated VMS RTL image.
 * No original DEC source text is included.
 */
#include "../vms_rtl_m108.h"
int port108_mthcabs(const uint16_t *pair,uint16_t *out){
 return port108_float_cabs(PORT_M107_F,pair,out);
}
