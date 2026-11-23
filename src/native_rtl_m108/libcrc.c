/* Original: RTL/SRC/LIBCRC.MAR at pinned DEC reference.
 * Reference reflected polynomial CRC, original 16-table VAX CRC hardware/table format still unresolved.
 * Host-tested IA-32 subset, NOT a complete translated VMS RTL image.
 * No original DEC source text is included.
 */
#include "../vms_rtl_m108.h"
uint32_t port108_libcrc(uint32_t polynomial,uint32_t initial,const uint8_t *data,size_t bytes){
 return port108_crc_reference(polynomial,initial,data,bytes);
}
