#ifndef VMS_SYSBOOT_MAP_H
#define VMS_SYSBOOT_MAP_H
#include <stdint.h>
#include <stddef.h>
#include "vms_boot_files11.h"
/* BOO$MAPVBN and BOO$SETMAP, SYSBOOT.MAR. One-origin file VBNs.
 * Extents use the original normalized count/LBN representation. */
uint32_t port_sysboot_mapvbn(const struct port_f11_extent *map,
                              uint32_t count,uint32_t vbn,
                              uint32_t *lbn,uint32_t *remaining,
                              uint32_t *next_index);
/* BOOTS/SRC/SYSBOOT.MAR BOO$L_SYS_MAP. The first longword holds
 * the byte length of normalized {32-bit block count, 32-bit LBN}
 * retrieval records. SYSBOOT reserves extra space for later INIT WCB
 * overlay; this builder does not claim a live WCB or pager. */
struct port_sysboot_normalized_map {
    const uint8_t *record;
    uint32_t extent_count;
    uint32_t normalized_bytes;
    uint32_t total_blocks;
};
uint32_t port_sysboot_build_normalized_map(uint8_t *destination,
    size_t capacity,const struct port_f11_extent *extents,
    uint32_t extent_count,uint32_t minimum_blocks,
    struct port_sysboot_normalized_map *out);
uint32_t port_sysboot_check_normalized_map(const uint8_t *record,
    size_t capacity,uint32_t minimum_blocks,
    struct port_sysboot_normalized_map *out);
uint32_t port_sysboot_setmap(const struct port_f11_extent *map,
                             uint32_t count,uint32_t first_vbn,
                             uint32_t requested,
                             struct port_f11_extent *result,
                             uint32_t capacity,uint32_t *used);
#endif
