/* Original: RTL/SRC/COMR50WD.MAR (read-only DEC reference).
 * Native IA-32 operation: Three-character native RADIX-50 word packer; invalid character scan semantics incomplete.
 * Not a complete VAX calling-convention port and not a guest-loaded RTL.
 * No original source text is distributed here.
 */
#include "../vms_rtl_m107.h"
int port_m107_comr50wd(const char *ascii,size_t n,uint16_t *out,size_t *used){
    return port_m107_rad50_word(ascii,n,out,used);
}
