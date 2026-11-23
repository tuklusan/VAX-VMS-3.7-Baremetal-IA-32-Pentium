#ifndef VMS_INIT_INDEX_H
#define VMS_INIT_INDEX_H
#include <stddef.h>
#include <stdint.h>
#include "vms_init_mfd.h"
#include "vms_init_bitmap.h"

/* INIT/SRC/ININDX.B32. The INIT allocation pass fills this layout. */
struct port_init_index_layout {
    uint32_t cluster,volume_blocks,max_files;
    uint32_t boot_lbn,boot_blocks,home1_lbn,home2_lbn;
    uint32_t real_home_lbn,alternate_index_lbn;
    uint32_t index_lbn,index_blocks,bitmap_lbn,bitmap_blocks;
    uint32_t mfd_lbn,mfd_blocks;
    uint32_t owner_uic,serial,protection,file_protection,record_protection;
    uint32_t extension;
    uint8_t window,lru_limit,read_check,write_check;
    uint64_t creation_time;
    char label[13],owner_name[13];
    const struct port_init_allocation *index_areas;
    size_t index_area_count;
    const struct port_init_allocation *bad_blocks;
    size_t bad_block_count;
};
uint32_t port_init_index(const struct port_init_index_layout *layout,
                         uint8_t buffer[512],port_init_write_block writer,
                         void *context);
#endif
