/* Original: RTL/SRC/LIBREMQHI.MAR at pinned DEC reference.
 * Source-ordered 32-bit queue links and empty/one statuses; interlocked retries not yet live.
 * Host-tested IA-32 subset, NOT a complete translated VMS RTL image.
 * No original DEC source text is included.
 */
#include "../vms_rtl_m108.h"
int port108_libremqhi(struct port108_queue *q,uint32_t *removed){
 return port108_queue_remove(q,removed,0);
}
