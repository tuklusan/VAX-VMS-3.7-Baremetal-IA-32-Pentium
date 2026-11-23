/* Original: RTL/SRC/MTHFLOOR.MAR at pinned DEC reference.
 * Native VAX F integral rounding; original register-entry/PSL variants remain unbound.
 * Host-tested IA-32 subset, NOT a complete translated VMS RTL image.
 * No original DEC source text is included.
 */
#include "../vms_rtl_m108.h"
int port108_mthfloor(const uint16_t *v,uint16_t *out){
    return port108_float_transform(PORT_M107_F,v,out,PORT108_FLOOR);
}
