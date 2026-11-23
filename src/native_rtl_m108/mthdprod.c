/* Original: RTL/SRC/MTHDPROD.MAR at pinned DEC reference.
 * Native exact 24-bit VAX F significand multiplication widened to D format.
 * Host-tested IA-32 subset, NOT a complete translated VMS RTL image.
 * No original DEC source text is included.
 */
#include "../vms_rtl_m108.h"
int port108_mthdprod(const uint16_t a[2],const uint16_t b[2],uint16_t *out){
    return port108_float_product(PORT_M107_D,a,b,out);
}
