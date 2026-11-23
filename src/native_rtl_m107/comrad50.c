/* Original: RTL/SRC/COMRAD50.MAR (read-only DEC reference).
 * Native IA-32 operation: Six-character RADIX-50 is two 16-bit words packed in DEC high-word first order.
 * Not a complete VAX calling-convention port and not a guest-loaded RTL.
 * No original source text is distributed here.
 */
#include "../vms_rtl_m107.h"
int port_m107_comrad50(const char *ascii,uint32_t *out){
    return port_m107_rad50_six(ascii,out);
}
