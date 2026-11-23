#ifndef VMS_BOOTIO_H
#define VMS_BOOTIO_H
#include <stddef.h>
#include <stdint.h>

/* BOOTS/SRC/BOOTIO.MAR (V03-000).  The bootstrap RPB selects the driver;
 * its I/O routine takes an LBN and a byte count, not an RMS file name. */
enum { BOO_IO_NORMAL = 1u, BOO_IO_BADPARAM = 2u,
       BOO_IO_FAILED = 4u, BOO_IO_NOMEM = 6u };
struct boo_io_driver {
    uint32_t (*transfer)(void *ctx, uint32_t mode, uint32_t function,
                         uint32_t lbn, uint32_t count, void *buffer);
    void *ctx;
    uint32_t rpb_address;
};
uint32_t boo_rdwr_lbn(const struct boo_io_driver *driver,
                      uint32_t lbn, void *buffer, uint32_t function,
                      uint32_t count);
uint32_t boo_assign(const struct boo_io_driver *driver, uint32_t *channel);

/* Good-page bitmap: bit one means the page is usable, as in the RPB. */
struct boo_page_map {
    const uint8_t *good;
    uint32_t bitmap_bytes;
    uint32_t first_pfn;
    uint32_t total_pages;
};
struct boo_cache {
    uint32_t page_count;
    uint32_t start_pfn;
    uint32_t open_status;
};
struct boo_cache_service {
    uint32_t (*init)(void *ctx, uint32_t cache_bytes, uint32_t cache_address,
                     uint32_t directory_blocks, uint32_t directory_entries,
                     uint32_t *returned_bytes, uint32_t *returned_address);
    void *ctx;
    uint32_t directory_entries;
};
int boo_alloc_pages(const struct boo_page_map *map, uint32_t start,
                    uint32_t desired, uint32_t minimum, uint32_t max_pfn,
                    uint32_t *found, uint32_t *base);
void boo_cache_alloc(const struct boo_page_map *map, struct boo_cache *cache);
uint32_t boo_cache_open(struct boo_cache *cache,
                        const struct boo_cache_service *service);
uint32_t boo_cache_init(const struct boo_page_map *map,
                        struct boo_cache *cache,
                        const struct boo_cache_service *service);

/* Header offsets are IHD/IHS/IHP constants from the loaded image's
 * structure definitions; all offsets are byte offsets from their bases. */
struct boo_image_layout {
    uint16_t debug_offset, patch_offset, header_count;
    uint16_t debug_vbn, global_vbn, patch_vbn;
};
int boo_image_attributes(const uint8_t *block, size_t bytes,
                         const struct boo_image_layout *layout,
                         uint32_t *image_last_vbn,
                         uint32_t *header_blocks);
/* FIL$GT_DDSTRING and FIL$GT_DDDEV, original counted strings. */
extern const uint8_t boo_default_directory[9];
extern const uint8_t boo_default_device[1];
#endif
