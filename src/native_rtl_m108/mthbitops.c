/* Original: RTL/SRC/MTHBITOPS.MAR at pinned DEC reference.
 * All ten integer AND, OR, XOR, NOT and shift entry points; no VAX entry frame.
 * Host-tested IA-32 subset, NOT a complete translated VMS RTL image.
 * No original DEC source text is included.
 */
#include "../vms_rtl_m108.h"
int port108_mthbitops_i2(unsigned operation,uint16_t a,uint16_t b,uint16_t *out){
 return port108_math_bit16(operation,a,b,out);
}
int port108_mthbitops_i4(unsigned operation,uint32_t a,uint32_t b,uint32_t *out){
 return port108_math_bit32(operation,a,b,out);
}
