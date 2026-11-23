/* Original: RTL/SRC/MTHDCONJG.MAR (read-only DEC reference).
 * Native IA-32 operation: Copies the real component and negates the imaginary sign, preserving zero.
 * Not a complete VAX calling-convention port and not a guest-loaded RTL.
 * No original source text is distributed here.
 */
#include "../vms_rtl_m107.h"
int port_m107_mthdconjg(const uint16_t *pair,uint16_t *out){
    return port_m107_float_conj(PORT_M107_D,pair,out);
}
