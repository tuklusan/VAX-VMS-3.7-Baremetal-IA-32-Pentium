#ifndef VMS_DQ_AUX_IMAGES_H
#define VMS_DQ_AUX_IMAGES_H
#include "vms_dq_imageprobe.h"
/* FIL$FINDFILID and FIL$READVBN for separately loaded native IA-32
 * bootstrap images. This is an internal privileged verification path,
 * not original VAX IMGACT, a Files-11 ACP, or a public driver. */
enum {PORT_DQ_AUX_OK=1u,PORT_DQ_AUX_BAD=2u,PORT_DQ_AUX_FORMAT=4u,
      PORT_DQ_AUX_MAP=6u};
struct port_dq_aux_segment {
    uint32_t va,offset,filesz,memsz,entry,sectors;
};
uint32_t port_dq_aux_descriptor(const uint8_t elf[512],
    uint32_t start,uint32_t end,struct port_dq_aux_segment *out);
uint32_t port_dq_aux_sector(const uint8_t fh2[512],
    const struct port_dq_aux_segment *seg,uint32_t index,
    uint32_t *lbn,uint32_t *va,uint32_t *valid);
#endif
