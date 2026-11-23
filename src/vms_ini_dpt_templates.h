#ifndef PORT_V37_DPT_TEMPLATES_H
#define PORT_V37_DPT_TEMPLATES_H
#include <stdint.h>
#include <stddef.h>
#include "vms_v37_dyn_tags.h"
/* Original SHRLIB/SYSMAR.MAR DPTAB at 334-385, SYSDEF.MDL 1290-1327.
 * 56-byte fixed prologue plus the empty init/reinit table terminator,
 * padded to 60 bytes for 32-bit IA-32 alignment. This is metadata,
 * NOT a full driver. DPT$W_SIZE=0 poisons IOC publication until real
 * linked driver END, INITAB, DDT/FDT, and adapter binding exist. */
enum {PORT_V37_DPT_PREFIX_BYTES=56u,PORT_V37_DPT_STAGE_BYTES=60u,
      PORT_V37_DPT_AT_UBA=1u,PORT_V37_DPT_MAXUNITS=8u,
      PORT_V37_DPT_DEFUNITS=1u,PORT_V37_DPT_COUNT=3u,
      PORT_V37_DPT_MB_UCBSIZE=132u,PORT_V37_DPT_NL_UCBSIZE=116u,
      PORT_V37_DPT_OP_UCBSIZE=188u};
uint32_t port_v37_dpt_fill(uint8_t *record,size_t capacity,
                          uint16_t ucb_bytes,const char *driver_name,
                          unsigned name_length);
uint32_t port_v37_dpt_stage_mb(uint8_t record[PORT_V37_DPT_STAGE_BYTES]);
uint32_t port_v37_dpt_stage_nl(uint8_t record[PORT_V37_DPT_STAGE_BYTES]);
uint32_t port_v37_dpt_stage_op(uint8_t record[PORT_V37_DPT_STAGE_BYTES]);
uint32_t port_v37_dpt_validate_staged(const uint8_t records[3][PORT_V37_DPT_STAGE_BYTES]);
#endif
