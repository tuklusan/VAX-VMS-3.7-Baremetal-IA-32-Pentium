#ifndef VMS_DQ_MFD_HEADER_H
#define VMS_DQ_MFD_HEADER_H
#include "vms_dq_index_header.h"
/* SYS/SRC/FILEREAD.MAR FIL$READVBN, FIL$RDCHKFILHDR:
 * indexed FID$C_MFD=4,4,0 through INDEXF.SYS' FH2 extent map. */
enum { PORT_DQ_MFD_OK=1u,PORT_DQ_MFD_BAD=2u,
       PORT_DQ_MFD_MAP=4u,PORT_DQ_MFD_HEADER=6u,
       PORT_DQ_MFD_IO=8u};
struct port_dq_mfd_result {uint32_t vbn,lbn,io_count,extent_count;
    uint16_t fid,sequence,volume,checksum;};
uint32_t port_dq_mfd_location(const uint8_t home[512],
       const uint8_t index_header[512],uint32_t *vbn,uint32_t *lbn);
uint32_t port_dq_mfd_validate(const uint16_t [256],struct port_dq_mfd_result *);
uint32_t port_dq_mfd_read(const struct port_dq_bootread_request *,
       const uint16_t home[256],const uint16_t index[256],
       struct port_dq_bootread_result *,struct port_dq_mfd_result *);
#endif
