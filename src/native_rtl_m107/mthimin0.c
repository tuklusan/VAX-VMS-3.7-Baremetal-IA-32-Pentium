/* Original: RTL/SRC/MTHIMIN0.MAR (read-only DEC reference).
 * Native IA-32 operation: Source count is the VAX low byte of the argument count; helper rejects zero and >255.
 * Not a complete VAX calling-convention port and not a guest-loaded RTL.
 * No original source text is distributed here.
 */
#include "../vms_rtl_m107.h"
int port_m107_mthimin0(const int16_t * const *a,size_t n,int16_t *out){
    return port_m107_int16_extreme(a,n,0,out);
}
