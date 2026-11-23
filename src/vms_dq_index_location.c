#include "vms_dq_index_header.h"
/* SHRLIB/F11DEF.B32 HM2 layout, as used by FIL$MOUNT. The readonly
 * home image must be complete and checksummed before any ATA I/O. */
uint32_t port_dq_index_location(const uint8_t home[512],uint32_t *lbn){
    if(!home||!lbn)return PORT_DQ_INDEX_BAD;
    if(home[13u]!=2u || port_dq_index_long(home,0u)!=1u ||
       !port_dq_index_long(home,8u)||!port_dq_index_word(home,14u)||
       !port_dq_index_word(home,16u)||!port_dq_index_word(home,18u)||
       !port_dq_index_word(home,20u)||!port_dq_index_word(home,22u)||
       !port_dq_index_long(home,28u)||!port_dq_index_word(home,34u)||
       port_dq_index_long(home,28u)<port_dq_index_word(home,34u)||
       !port_dq_index_checksum(home,58u)||
       !port_dq_index_checksum(home,510u))return PORT_DQ_INDEX_HOME;
    const uint32_t bitmap=port_dq_index_long(home,24u);
    const uint32_t blocks=port_dq_index_word(home,32u);
    if(!bitmap||!blocks||bitmap>=UINT32_C(0x10000000)||
       (uint64_t)bitmap+blocks>UINT32_C(0x0fffffff))
        return PORT_DQ_INDEX_HOME;
    *lbn=bitmap+blocks;
    return PORT_DQ_INDEX_OK;
}
