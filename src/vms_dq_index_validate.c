#include "vms_dq_index_header.h"
/* FILE HEADER INDEXF.SYS;1 must have FH2$W_FID_NUM=1, SEQ=1,
 * RVN=0 and a correct FH2 checksum. Never infer success from I/O alone. */
uint32_t port_dq_index_validate_header(const uint16_t words[256],
                                      struct port_dq_index_result *out){
    if(!words||!out)return PORT_DQ_INDEX_BAD;
    const uint8_t *h=(const uint8_t *)words;
    if(h[7u]!=2u||port_dq_index_word(h,8u)!=1u||
       port_dq_index_word(h,10u)!=1u||port_dq_index_word(h,12u)!=0u||
       h[0u]==0u||h[1u]<=h[0u]||h[1u]>=255u||
       !port_dq_index_checksum(h,510u))return PORT_DQ_INDEX_HEADER;
    out->header_checksum=port_dq_index_word(h,510u);
    out->file_number=port_dq_index_word(h,8u);
    out->file_sequence=port_dq_index_word(h,10u);
    out->record_bytes=512u;
    return PORT_DQ_INDEX_OK;
}
