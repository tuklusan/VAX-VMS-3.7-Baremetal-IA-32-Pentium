/* Original: RTL/SRC/LIBMOVTC.MAR at pinned DEC reference.
 * Byte translation plus fill for fixed descriptors; dynamic descriptor and overlap unresolved.
 * Host-tested IA-32 subset, NOT a complete translated VMS RTL image.
 * No original DEC source text is included.
 */
#include "../vms_rtl_m108.h"
int port108_libmovtc(const uint8_t *src,size_t ns,const uint8_t table[256],uint8_t fill,uint8_t *dest,size_t nd){
 return port108_movtc(src,ns,table,fill,dest,nd);
}
