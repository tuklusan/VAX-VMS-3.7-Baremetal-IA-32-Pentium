#ifndef VMS_EXE_PAGED_POOL_H
#define VMS_EXE_PAGED_POOL_H
#include <stddef.h>
#include <stdint.h>
#include "vms_exe_np_pool.h"

/* Source EXE$ALOPAGED/EXE$DEAPAGED boundary over a fixed, already resident
 * pool. Page backing/extension and SCH$LOCKW wait/wakeup remain separate. */
enum {PORT_EXE_PAGED_OK=1u,PORT_EXE_PAGED_BAD=2u,
      PORT_EXE_PAGED_EMPTY=4u,PORT_EXE_PAGED_CORRUPT=8u,
      PORT_EXE_PAGED_DUP=16u};
struct port_exe_paged_pool { struct port_exe_np_pool free_blocks; };
uint32_t port_exe_paged_pool_init(struct port_exe_paged_pool *,void *,size_t,
    const struct port_v37_irp_pool_sync *);
uint32_t port_exe_paged_alloc(struct port_exe_paged_pool *,uint32_t,
    void **,uint32_t *);
uint32_t port_exe_paged_free(struct port_exe_paged_pool *,void *,uint32_t);
#endif
