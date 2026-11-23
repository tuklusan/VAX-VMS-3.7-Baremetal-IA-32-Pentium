#include "vms_bootio.h"
const uint8_t boo_default_directory[9]={8u,'[','S','Y','S','E','X','E',']'};
const uint8_t boo_default_device[1]={0u};

uint32_t boo_rdwr_lbn(const struct boo_io_driver *driver,
                      uint32_t lbn, void *buffer, uint32_t function,
                      uint32_t count) {
    if (!driver || !driver->transfer || (!buffer && count))
        return BOO_IO_BADPARAM;
    /* FIL$RDWRTLBN passes mode zero for a physical buffer. */
    return driver->transfer(driver->ctx, 0u, function, lbn, count, buffer);
}
uint32_t boo_assign(const struct boo_io_driver *driver, uint32_t *channel) {
    if (!driver || !channel) return BOO_IO_BADPARAM;
    /* SYS$ASSIGN in BOOTIO intentionally returns the RPB address. */
    *channel = driver->rpb_address;
    return BOO_IO_NORMAL;
}

static int page_good(const struct boo_page_map *map, uint32_t pfn) {
    if (!map || !map->good || pfn < map->first_pfn) return 0;
    uint32_t bit = pfn-map->first_pfn;
    if (bit >= map->total_pages || bit >= map->bitmap_bytes * 8u)
        return 0;
    return !!(map->good[bit>>3u] & (1u << (bit&7u)));
}
int boo_alloc_pages(const struct boo_page_map *map, uint32_t start,
                    uint32_t desired, uint32_t minimum, uint32_t max_pfn,
                    uint32_t *found, uint32_t *base) {
    if (!found || !base) return 0;
    *found = *base = 0;
    if (!map || !map->good || !desired || !minimum ||
        minimum > desired || start >= max_pfn)
        return 0;
    uint32_t run_start = start;
    uint32_t pfn=start;
    /* BOO$ALLOC_PAGES scans at most the requested count of PFNs.
     * It returns on a broken run once half the request is found. */
    for (uint32_t n=0;n<desired && pfn<max_pfn;n++,pfn++) {
        if (!page_good(map,pfn)) {
            uint32_t size=pfn-run_start;
            if (size>=minimum) {
                *found=size;
                *base=run_start;
                return 1;
            }
            run_start=pfn+1u;
        }
    }
    if (pfn-run_start>=minimum) {
        *found=pfn-run_start;
        *base=run_start;
        return 1;
    }
    return 0;
}

/* MEM_CACHE_TABLE is copied as numeric values, not DEC source code. */
struct cache_rule { uint32_t minimum_ram, search_begin, wanted, search_end; };
static const struct cache_rule cache_rules[] = {
    {16384u-16384u/10u,2048u,64u,4096u},
    {8192u-8192u/10u,1024u,64u,2048u},
    {4096u-4096u/10u,640u,64u,1024u},
    {2048u-2048u/10u,512u,64u,768u},
    {1024u-1024u/10u,256u,32u,512u},
    {512u-512u/10u,256u,16u,256u},
    {384u-384u/10u,256u,8u,192u},
    {256u-256u/10u,128u,4u,128u}
};
void boo_cache_alloc(const struct boo_page_map *map, struct boo_cache *cache) {
    if (!cache) return;
    cache->page_count=cache->start_pfn=cache->open_status=0;
    if (!map) return;
    for (unsigned i=0;i<sizeof(cache_rules)/sizeof(cache_rules[0]);++i) {
        const struct cache_rule *rule=&cache_rules[i];
        if (map->total_pages<rule->minimum_ram) continue;
        uint32_t start=map->first_pfn+rule->search_begin;
        uint32_t finish=map->first_pfn+rule->search_end;
        if (finish<start) continue;
        if (boo_alloc_pages(map,start,rule->wanted,rule->wanted/2u,
                            finish,&cache->page_count,&cache->start_pfn))
            return;
        /* The VAX code selects only its first size-matching rule. */
        return;
    }
}
uint32_t boo_cache_open(struct boo_cache *cache,
                        const struct boo_cache_service *service) {
    if (!cache) return BOO_IO_BADPARAM;
    if (!cache->page_count) return BOO_IO_NORMAL;
    if (!service || !service->init || cache->page_count < 2u ||
        cache->start_pfn > UINT32_MAX/512u)
        return BOO_IO_BADPARAM;
    uint32_t bytes=cache->page_count * 512u;
    uint32_t location=cache->start_pfn * 512u;
    uint32_t rc=service->init(service->ctx,bytes,location,
                             cache->page_count-2u,service->directory_entries,
                             &bytes,&location);
    cache->open_status=rc;
    if (rc&1u) {
        cache->page_count=bytes/512u;
        cache->start_pfn=location/512u;
    }
    return rc;
}
uint32_t boo_cache_init(const struct boo_page_map *map,
                        struct boo_cache *cache,
                        const struct boo_cache_service *service) {
    if (!cache) return BOO_IO_BADPARAM;
    boo_cache_alloc(map,cache);
    return boo_cache_open(cache,service);
}

static uint16_t ld16(const uint8_t *p) {
    return (uint16_t)(p[0] | (uint16_t)(p[1]<<8u));
}
static uint32_t ld32(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1]<<8u) |
           ((uint32_t)p[2]<<16u) | ((uint32_t)p[3]<<24u);
}
static int trim_image(uint32_t *last,const uint8_t *block,size_t bytes,
                      uint32_t base,uint32_t offset) {
    if (base > bytes || offset > bytes-base || 4u > bytes-base-offset)
        return 0;
    uint32_t vbn=ld32(block+base+offset);
    if (vbn && vbn-1u < *last) *last=vbn-1u;
    return 1;
}
int boo_image_attributes(const uint8_t *block, size_t bytes,
                         const struct boo_image_layout *layout,
                         uint32_t *image_last_vbn,
                         uint32_t *header_blocks) {
    if (!block || !layout || !image_last_vbn || !header_blocks ||
        layout->debug_offset + 2u > bytes ||
        layout->patch_offset + 2u > bytes ||
        layout->header_count >= bytes)
        return 0;
    uint32_t last=*image_last_vbn;
    uint32_t debug=ld16(block+layout->debug_offset);
    uint32_t patch=ld16(block+layout->patch_offset);
    if (debug && (!trim_image(&last,block,bytes,debug,layout->debug_vbn) ||
                  !trim_image(&last,block,bytes,debug,layout->global_vbn)))
        return 0;
    if (patch && !trim_image(&last,block,bytes,patch,layout->patch_vbn))
        return 0;
    *image_last_vbn=last;
    *header_blocks=block[layout->header_count];
    return 1;
}
