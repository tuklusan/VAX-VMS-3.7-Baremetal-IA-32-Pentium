/* Original: RTL/SRC/LIBEDIV.MAR at pinned DEC reference.
 * EDIV quotient/remainder; VAX INTDIV/INTOVF condition handler integration pending.
 * Host-tested IA-32 subset, NOT a complete translated VMS RTL image.
 * No original DEC source text is included.
 */
#include "../vms_rtl_m108.h"
int port108_libediv(int32_t divisor,int64_t dividend,int32_t *quot,int32_t *rem){
 return port108_ediv(divisor,dividend,quot,rem);
}
