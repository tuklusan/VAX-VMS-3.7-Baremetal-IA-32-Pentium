/* Original: RTL/SRC/MTHIISIGN.MAR (read-only DEC reference).
 * Native IA-32 operation: The VAX MNEG operation wraps at the narrow signed minimum; no host signed overflow.
 * Not a complete VAX calling-convention port and not a guest-loaded RTL.
 * No original source text is distributed here.
 */
#include "../vms_rtl_m107.h"
int port_m107_mthiisign(int16_t magnitude,int16_t sign,int16_t *out){
    return port_m107_int16_sign(magnitude,sign,out);
}
