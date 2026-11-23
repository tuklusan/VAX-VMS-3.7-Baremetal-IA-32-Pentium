/* Original: RTL/SRC/LIBEMUL.MAR at pinned DEC reference.
 * EMUL exact signed 32x32 plus 32-bit addend to 64-bit result.
 * Host-tested IA-32 subset, NOT a complete translated VMS RTL image.
 * No original DEC source text is included.
 */
#include "../vms_rtl_m108.h"
int port108_libemul(int32_t multiplier,int32_t multiplicand,int32_t addend,int64_t *result){
 return port108_emul(multiplier,multiplicand,addend,result);
}
