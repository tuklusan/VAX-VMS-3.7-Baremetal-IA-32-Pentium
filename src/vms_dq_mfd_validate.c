#include "vms_dq_mfd_header.h"
/* Original FIL$RDCHKFILHDR FID MFD 4,4,0 and 16-bit FH2 checksum. */
uint32_t port_dq_mfd_validate(const uint16_t words[256],
                             struct port_dq_mfd_result *r){
    if(!words||!r)return PORT_DQ_MFD_BAD;
    const uint8_t *b=(const uint8_t *)words;
    if(b[7u]!=2u||port_dq_index_word(b,8u)!=4u||
       port_dq_index_word(b,10u)!=4u||
       port_dq_index_word(b,12u)!=0u||
       b[0u]==0u||b[1u]<=b[0u]||b[1u]>=255u||
       !port_dq_index_checksum(b,510u))return PORT_DQ_MFD_HEADER;
    r->fid=4u;r->sequence=4u;r->volume=0u;
    r->checksum=port_dq_index_word(b,510u);
    return PORT_DQ_MFD_OK;
}
