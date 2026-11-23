/* Original: RTL/SRC/MTHGSIGN.MAR (read-only DEC reference).
 * Native IA-32 operation: Sign and exponent read from the low-addressed VAX word, not a host IEEE float.
 * Not a complete VAX calling-convention port and not a guest-loaded RTL.
 * No original source text is distributed here.
 */
#include "../vms_rtl_m107.h"
int port_m107_mthgsign(const uint16_t *a,const uint16_t *b,uint16_t *out){
    return port_m107_float_sign(PORT_M107_G,a,b,out);
}
