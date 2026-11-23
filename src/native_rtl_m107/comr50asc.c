/* Original: RTL/SRC/COMR50ASC.MAR (read-only DEC reference).
 * Native IA-32 operation: RADIX-50 inverse for all 40 codes, including unused code 29.
 * Not a complete VAX calling-convention port and not a guest-loaded RTL.
 * No original source text is distributed here.
 */
#include "../vms_rtl_m107.h"
int port_m107_comr50asc(const uint16_t *encoded,size_t n,char *ascii,size_t capacity){
    return port_m107_r50asc(encoded,n,ascii,capacity);
}
