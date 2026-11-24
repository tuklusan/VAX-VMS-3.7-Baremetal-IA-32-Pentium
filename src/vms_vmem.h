#ifndef PORTLAB_VMS_VMEM_H
#define PORTLAB_VMS_VMEM_H
#include <stdint.h>
#include "vms_phys.h"
#include "vms_pmap.h"

/* M8: bounded per-process demand-zero VA region. IA-32 PTEs, not VAX PTEs.
 * All pages are within the supervisor identity aperture (< 4 MiB).
 * The caller must hold the scheduler/VM lock (single CPU: trap gate IF=0).
 * The physical-page zero callback is required BEFORE a user mapping appears. */
enum { PORT_VMEM_MAX_PAGES = 8u, PORT_VMEM_INVALID = -1,
       PORT_VMEM_OOM = -2, PORT_VMEM_PROTECTION = -3,
       PORT_VMEM_LOCKED = -4,
       PORT_VMEM_HANDLED = 1 };
struct port_vmem_region {
    uint32_t start;
    uint32_t count;
    uint32_t frames[PORT_VMEM_MAX_PAGES];
    uint32_t faults;
    uint32_t denied;
};
typedef void (*port_vmem_zero_fn)(uint32_t physical_address, void *ctx);
int port_vmem_reserve(struct port_vmem_region *r,
                      uint32_t pt[PORT_PMAP_ENTRIES],
                      uint32_t start, uint32_t count);
int port_vmem_fault(struct port_vmem_region *r,
                    struct port_phys_pool *pool,
                    uint32_t pt[PORT_PMAP_ENTRIES],
                    uint32_t va, uint32_t x86_error,
                    port_vmem_zero_fn zero_page, void *ctx);
int port_vmem_release(struct port_vmem_region *r,
                      struct port_phys_pool *pool,
                      uint32_t pt[PORT_PMAP_ENTRIES]);
uint32_t port_vmem_committed(const struct port_vmem_region *r);
#endif
