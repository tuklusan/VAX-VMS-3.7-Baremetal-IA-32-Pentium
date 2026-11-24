#ifndef VMS_EXE_NP_POOL_H
#define VMS_EXE_NP_POOL_H
#include <stddef.h>
#include <stdint.h>
#include "vms_qio_irp_pool.h"

/* SYS/SRC/MEMORYALC.MAR EXE$ALLOCATE over a preinitialized nonpaged region.
 * Free blocks remain address sorted; allocation is source first-fit and
 * splitting, and deallocation coalesces adjacent blocks. Pool extension and
 * resource-wait delivery are separate executive services. */
enum {PORT_EXE_NP_POOL_OK=1u,PORT_EXE_NP_POOL_BAD=2u,
      PORT_EXE_NP_POOL_EMPTY=4u,PORT_EXE_NP_POOL_CORRUPT=8u,
      PORT_EXE_NP_POOL_DUP=16u};
/* IA-32 pool links are stored as byte offsets so host verification preserves
 * the source packet's eight-byte layout on 64-bit test machines. */
struct port_exe_np_free {uint32_t next_offset,bytes;};
struct port_exe_np_pool {
    uint8_t *base;
    uint32_t bytes;
    uint32_t head_offset;
    uint32_t allocations;
    const struct port_v37_irp_pool_sync *sync;
};
struct port_exe_np_allocator {
    struct port_ini_packet_list *irps,*lrps,*srps;
    struct port_exe_np_pool *variable;
    const struct port_v37_irp_pool_sync *sync;
};
enum {PORT_EXE_NP_FROM_VARIABLE=0u,PORT_EXE_NP_FROM_IRP=1u,
      PORT_EXE_NP_FROM_LRP=2u,PORT_EXE_NP_FROM_SRP=3u};
struct port_exe_np_allocation {
    void *address;
    uint32_t bytes;
    uint8_t source,tag;
};
uint32_t port_exe_np_pool_init(struct port_exe_np_pool *,void *,size_t,
    const struct port_v37_irp_pool_sync *);
uint32_t port_exe_np_pool_alloc(struct port_exe_np_pool *,uint32_t,
    void **,uint32_t *);
uint32_t port_exe_np_pool_free(struct port_exe_np_pool *,void *,uint32_t);
uint32_t port_exe_np_pool_check(const struct port_exe_np_pool *);
uint32_t port_exe_np_allocate(struct port_exe_np_allocator *,uint32_t,uint8_t,
    struct port_exe_np_allocation *);
uint32_t port_exe_np_release(struct port_exe_np_allocator *,
    const struct port_exe_np_allocation *);
uint32_t port_exe_np_deallocate(struct port_exe_np_allocator *,void *,
    uint32_t,uint8_t);
#endif
