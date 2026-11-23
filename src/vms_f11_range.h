#ifndef VMS_F11_RANGE_H
#define VMS_F11_RANGE_H
#include "vms_dq_dir.h"
#include <stddef.h>
/* FIL$READVBN byte-span semantics on a source Files-11 level-2 FH2.
 * Physical callback is privileged boot ownership, NOT a user-mode ACP. */
enum {PORT_F11_RANGE_OK=1u,PORT_F11_RANGE_BAD=2u,
      PORT_F11_RANGE_MAP=4u,PORT_F11_RANGE_IO=6u};
uint32_t port_f11_read_range(const uint8_t fh2[512],uint32_t file_offset,
    uint32_t count,uint8_t *destination,size_t capacity,
    port_dq_dir_sector_read_fn read_sector,void *context,
    uint32_t *completed);
#endif
