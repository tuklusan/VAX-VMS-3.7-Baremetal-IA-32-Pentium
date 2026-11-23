#include "vms_dq_bootread.h"
/* MOUNT/SRC/CHKSM2.B32: original Files-11 ODS2 home-checksum semantics.
 * No fake signature: checksum the bytes returned by the real ATA read. */
uint16_t port_dq_bootread_checksum(const uint16_t words[256]){
    if(!words)return 0u;
    uint16_t s=0u;
    for(unsigned i=0;i<255u;i++)s=(uint16_t)(s+words[i]);
    return s;
}
