/* Original: RTL/SRC/LIBGETOPC.MAR at pinned DEC reference.
 * VAX one- or two-byte opcode decode; signal handler and mapped access unbound.
 * Host-tested IA-32 subset, NOT a complete translated VMS RTL image.
 * No original DEC source text is included.
 */
#include "../vms_rtl_m108.h"
int port108_libgetopc(const uint8_t *instruction,size_t readable,uint16_t *opcode){
 return port108_get_opcode(instruction,readable,opcode);
}
