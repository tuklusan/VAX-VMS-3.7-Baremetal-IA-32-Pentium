/* Original: RTL/SRC/COMIRAD50.MAR (read-only DEC reference).
 * Native IA-32 operation: Caller-sized RADIX-50 array; full VAX descriptor arguments not yet integrated.
 * Not a complete VAX calling-convention port and not a guest-loaded RTL.
 * No original source text is distributed here.
 */
#include "../vms_rtl_m107.h"
int port_m107_comirad50(const char *ascii,size_t n,uint16_t *out,size_t capacity,size_t *consumed){
    return port_m107_irad50(ascii,n,out,capacity,consumed);
}
