#ifndef VMS_DQ_INDEX_HEADER_H
#define VMS_DQ_INDEX_HEADER_H
#include "vms_dq_bootread.h"
/* SYS/SRC/FILEREAD.MAR FIL$MOUNT: find INDEXF.SYS' primary FH2 using
 * HM2$L_IBMAPLBN + HM2$W_IBMAPSIZE. This does not activate an ACP. */
enum {PORT_DQ_INDEX_OK=1u,PORT_DQ_INDEX_BAD=2u,
      PORT_DQ_INDEX_HOME=4u,PORT_DQ_INDEX_HEADER=6u,
      PORT_DQ_INDEX_IO=8u};
struct port_dq_index_result {
    uint32_t lbn,bitmap_lbn,bitmap_blocks,io_count;
    uint16_t header_checksum,file_number,file_sequence,record_bytes;
};
uint16_t port_dq_index_word(const uint8_t *,size_t);
uint32_t port_dq_index_long(const uint8_t *,size_t);
int port_dq_index_checksum(const uint8_t [512],size_t);
uint32_t port_dq_index_location(const uint8_t [512],uint32_t *);
uint32_t port_dq_index_validate_header(const uint16_t [256],
                                      struct port_dq_index_result *);
uint32_t port_dq_index_read(const struct port_dq_bootread_request *,
                           const uint16_t [256],
                           struct port_dq_bootread_result *,
                           struct port_dq_index_result *);
#endif
