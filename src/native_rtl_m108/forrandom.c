/* Original: RTL/SRC/FORRANDOM.MAR at pinned DEC reference.
 * Original 65539 two-word seed transition; F fraction precision and first-call handling partial.
 * Host-tested IA-32 subset, NOT a complete translated VMS RTL image.
 * No original DEC source text is included.
 */
#include "../vms_rtl_m108.h"
int port108_forrandom(uint16_t *high,uint16_t *low,uint16_t out[2]){
 return port108_random_fortran(high,low,out);
}
