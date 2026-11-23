/* Original: RTL/SRC/MTHRANDOM.MAR at pinned DEC reference.
 * Original 69069 state transition and high-24 fraction as VAX F encoding.
 * Host-tested IA-32 subset, NOT a complete translated VMS RTL image.
 * No original DEC source text is included.
 */
#include "../vms_rtl_m108.h"
int port108_mthrandom(uint32_t *seed,uint16_t out[2]){
 return port108_random_mth(seed,out);
}
