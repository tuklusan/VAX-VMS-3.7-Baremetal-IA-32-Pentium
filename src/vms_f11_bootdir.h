#ifndef VMS37_F11_BOOTDIR_H
#define VMS37_F11_BOOTDIR_H
#include "vms_f11_bootpath.h"
/* Read-only source Files-11 directory scan; callback borrows the entry
 * name only for its duration and may stop early by returning zero.
 * No RMS name parser, ACP lock, directory cache or process channel. */
typedef uint32_t (*port_f11_bootdir_visit_fn)(void *ctx,
    const uint8_t *name,size_t length,uint16_t version,
    const struct port_dq_dir_fid *fid);
enum { PORT_F11_BOOTDIR_OK=1u, PORT_F11_BOOTDIR_BAD=2u,
       PORT_F11_BOOTDIR_CORRUPT=4u, PORT_F11_BOOTDIR_MAP=6u,
       PORT_F11_BOOTDIR_STOP=8u };
uint32_t port_f11_bootdir_block(const uint8_t block[512],
    port_f11_bootdir_visit_fn visit,void *ctx,uint32_t *seen);
uint32_t port_f11_bootdir_walk(const struct port_f11_boot_volume *,
    const uint8_t directory_fh2[512],port_f11_bootdir_visit_fn visit,
    void *ctx,uint32_t *seen);
#endif
