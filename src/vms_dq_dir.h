#ifndef VMS_DQ_DIR_H
#define VMS_DQ_DIR_H
#include "vms_dq_mfd_header.h"
/* SYS/SRC/FILEREAD.MAR FIL$READVBN and FIL$FINDFILID; 512-byte
 * level-2 directory blocks, exact FID/sequence/volume identities. */
enum {PORT_DQ_DIR_OK=1u,PORT_DQ_DIR_BAD=2u,
      PORT_DQ_DIR_MAP=4u,PORT_DQ_DIR_CORRUPT=6u,
      PORT_DQ_DIR_NOT_FOUND=8u};
struct port_dq_dir_fid {uint16_t number,sequence,volume;};
uint32_t port_dq_dir_map_vbn(const uint8_t header[512],uint32_t vbn,
                             uint32_t *lbn);
uint32_t port_dq_dir_find(const uint8_t block[512],const char *name,
    size_t length,uint16_t version,struct port_dq_dir_fid *fid);
uint32_t port_dq_dir_validate_header(const uint8_t block[512],
    const struct port_dq_dir_fid *fid);
/* Source FIL$FINDFILID directory search must not stop after the first
 * directory VBN. Restricted to read-only registered native physical I/O;
 * no process-level ACP channel is constructed here. */
typedef uint32_t (*port_dq_dir_sector_read_fn)(void *context,
    uint32_t physical_lbn,uint8_t sector[512]);
uint32_t port_dq_dir_find_blocks(const uint8_t fh2[512],
    const char *name,size_t length,uint16_t version,uint32_t max_vbn,
    port_dq_dir_sector_read_fn read_sector,void *context,
    struct port_dq_dir_fid *out,uint32_t *found_vbn);
#endif
