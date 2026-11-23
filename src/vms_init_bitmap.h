#ifndef VMS_INIT_BITMAP_H
#define VMS_INIT_BITMAP_H
#include <stddef.h>
#include <stdint.h>
#include "vms_init_mfd.h"

/* INIT/SRC/INIBIT.B32: INIT_BITMAP allocation-table input. */
struct port_init_allocation { uint32_t start_lbn, block_count; };
struct port_init_bitmap_options {
    unsigned structure1;
    uint32_t bitmap_lbn, bitmap_blocks, volume_blocks;
    uint32_t cluster, sectors_per_track, tracks_per_cylinder, cylinders;
};
uint32_t port_init_bitmap(const struct port_init_bitmap_options *opt,
                          const struct port_init_allocation *allocation,
                          size_t count,uint8_t buffer[512],
                          port_init_write_block write_block,void *context);
#endif
