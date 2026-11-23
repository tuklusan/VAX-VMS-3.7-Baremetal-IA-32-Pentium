/* Original: RTL/SRC/MTHJMIN0.MAR (read-only DEC reference).
 * Native IA-32 operation: Source count is the VAX low byte of the argument count; helper rejects zero and >255.
 * Not a complete VAX calling-convention port and not a guest-loaded RTL.
 * No original source text is distributed here.
 */
#include "../vms_rtl_m107.h"
int port_m107_mthjmin0(const int32_t * const *a,size_t n,int32_t *out){
    return port_m107_int32_extreme(a,n,0,out);
}
