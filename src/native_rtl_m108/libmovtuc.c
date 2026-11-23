/* Original: RTL/SRC/LIBMOVTUC.MAR at pinned DEC reference.
 * MOVTUC escape index with translated table, fixed destination only.
 * Host-tested IA-32 subset, NOT a complete translated VMS RTL image.
 * No original DEC source text is included.
 */
#include "../vms_rtl_m108.h"
int port108_libmovtuc(const uint8_t *src,size_t ns,const uint8_t table[256],uint8_t escape,uint8_t fill,uint8_t *dest,size_t nd,size_t *index){
 return port108_movtuc(src,ns,table,escape,fill,dest,nd,index);
}
