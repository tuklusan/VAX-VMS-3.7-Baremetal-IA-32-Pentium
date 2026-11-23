#include "vms_dq_dir.h"
/* FIL$RDCHKFILHDR verifies the physical FID and FH2 checksum before
 * interpreting any directory VBN. */
uint32_t port_dq_dir_validate_header(const uint8_t b[512],
                                    const struct port_dq_dir_fid *fid){
    if(!b||!fid||!fid->number||!fid->sequence)return PORT_DQ_DIR_BAD;
    if(b[7u]!=2u||b[0u]==0u||b[1u]<=b[0u]||b[1u]>=255u||
       port_dq_index_word(b,8u)!=fid->number||
       port_dq_index_word(b,10u)!=fid->sequence||
       port_dq_index_word(b,12u)!=fid->volume||
       !(port_dq_index_long(b,52u)&(1u<<13u))||
       b[20u]!=2u||b[21u]!=8u||
       !port_dq_index_checksum(b,510u))return PORT_DQ_DIR_CORRUPT;
    return PORT_DQ_DIR_OK;
}
