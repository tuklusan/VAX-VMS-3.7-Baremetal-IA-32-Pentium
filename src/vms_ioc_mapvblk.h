#ifndef VMS_IOC_MAPVBLK_H
#define VMS_IOC_MAPVBLK_H
#include <stddef.h>
#include <stdint.h>

enum {
    PORT_IOC_MAPVBLK_OK=1u,
    PORT_IOC_MAPVBLK_UNMAPPED=2u,
    PORT_IOC_MAPVBLK_BADPARAM=4u,
    PORT_IOC_MAPVBLK_BADWCB=6u,
    PORT_IOC_MAPVBLK_CORRUPT=8u,
    PORT_IOC_MAPVBLK_BADRVN=10u,
    PORT_IOC_MAPVBLK_WCB_BYTES=48u,
    PORT_IOC_MAPVBLK_TYPE_OFFSET=10u,
    PORT_IOC_MAPVBLK_ACCESS_OFFSET=11u,
    PORT_IOC_MAPVBLK_ORGUCB_OFFSET=16u,
    PORT_IOC_MAPVBLK_NMAP_OFFSET=22u,
    PORT_IOC_MAPVBLK_FCB_OFFSET=24u,
    PORT_IOC_MAPVBLK_RVT_OFFSET=28u,
    PORT_IOC_MAPVBLK_LINK_OFFSET=32u,
    PORT_IOC_MAPVBLK_STVBN_OFFSET=44u,
    PORT_IOC_MAPVBLK_ACCESS_NOTFCP=1u<<2
};

struct port_ioc_mapvblk_ops {
    void *context;
    const uint8_t *(*resolve_wcb)(void *,uint32_t,size_t,size_t *);
    uint32_t (*enter)(void *,uint32_t,uint32_t *);
    void (*leave)(void *,uint32_t,uint32_t);
    uint32_t (*is_sequential)(void *,uint32_t);
    uint32_t (*get_file_size)(void *,uint32_t,uint32_t *);
    uint32_t (*relative_ucb)(void *,uint32_t,uint8_t,uint32_t *);
    void (*record_hit)(void *);
};
struct port_ioc_mapvblk_result {
    uint32_t lbn,unmapped_bytes,ucb;
};

/* Native IOC$MAPVBLK: selects a linked WCB segment, walks FM2 retrieval
 * pointers, coalesces one adjacent extent, limits to FCB EOF and resolves RVN. */
uint32_t port_ioc_mapvblk(uint32_t,uint32_t,uint32_t,uint32_t,
    const struct port_ioc_mapvblk_ops *,struct port_ioc_mapvblk_result *);
#endif
