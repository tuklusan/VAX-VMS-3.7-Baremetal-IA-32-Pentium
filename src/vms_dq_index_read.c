#include "vms_dq_index_header.h"
/* Separate resident bootstrap IRP, genuine disk READPBLK, and structurally
 * validated INDEXF.SYS first file header; no DDT publication or ACP. */
uint32_t port_dq_index_read(const struct port_dq_bootread_request *q,
                           const uint16_t home[256],
                           struct port_dq_bootread_result *io,
                           struct port_dq_index_result *result){
    if(!q||!home||!io||!result||!q->irp)return PORT_DQ_INDEX_BAD;
    uint32_t lbn=0u;
    uint32_t rc=port_dq_index_location((const uint8_t *)home,&lbn);
    if(rc!=PORT_DQ_INDEX_OK)return rc;
    /* The IRP was independently prepared by privileged INIT. Its media
     * field must match exactly; do not silently repair caller input. */
    if(port_v37_read32(q->irp+PORT_V37_IRP_MEDIA)!=lbn)
        return PORT_DQ_INDEX_BAD;
    rc=port_dq_bootread_execute_physical(q,io,lbn);
    if(rc!=PORT_DQ_BOOTREAD_OK)return PORT_DQ_INDEX_IO;
    struct port_dq_index_result tmp={0};
    rc=port_dq_index_validate_header(q->sector,&tmp);
    if(rc!=PORT_DQ_INDEX_OK)return rc;
    tmp.lbn=lbn;
    tmp.bitmap_lbn=port_dq_index_long((const uint8_t *)home,24u);
    tmp.bitmap_blocks=port_dq_index_word((const uint8_t *)home,32u);
    tmp.io_count=io->ucb_io_count;
    *result=tmp;
    return PORT_DQ_INDEX_OK;
}
