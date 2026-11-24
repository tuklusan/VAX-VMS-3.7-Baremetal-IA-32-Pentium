#include "vms_vmem.h"

/* Exactly one PDE0 page table, with user privilege granted only by valid PTEs. */
int port_vmem_reserve(struct port_vmem_region *r,
                      uint32_t pt[PORT_PMAP_ENTRIES],
                      uint32_t start, uint32_t count) {
    if (!r || !pt || r->count || !count || count>PORT_VMEM_MAX_PAGES ||
        (start&(PORT_PAGE_BYTES-1u)) ||
        start==0 || start>=0x400000u ||
        count > (0x400000u-start)/PORT_PAGE_BYTES)
        return PORT_VMEM_INVALID;
    /* Never silently overwrite an existing user mapping. Require supervisor
     * identity PTEs (the existing fixed-image mapping) before reserving. */
    for (uint32_t i=0;i<count;i++) {
        uint32_t va=start+i*PORT_PAGE_BYTES;
        if (pt[va/PORT_PAGE_BYTES]!=(va|3u)) return PORT_VMEM_INVALID;
    }
    r->start=start;
    r->count=count;
    r->faults=r->denied=0;
    for (uint32_t i=0;i<PORT_VMEM_MAX_PAGES;i++) r->frames[i]=0;
    for (uint32_t i=0;i<count;i++)
        pt[(start/PORT_PAGE_BYTES)+i]=0u; /* nonpresent, no user access */
    return 0;
}

int port_vmem_fault(struct port_vmem_region *r,
                    struct port_phys_pool *pool,
                    uint32_t pt[PORT_PMAP_ENTRIES],
                    uint32_t va, uint32_t x86_error,
                    port_vmem_zero_fn zero_page, void *ctx) {
    if (!r || !pt || !pool || !r->count || !zero_page ||
        va<r->start || (va-r->start)/PORT_PAGE_BYTES>=r->count)
        return PORT_VMEM_INVALID;
    /* x86 #PF: P=0, U=1, W either. No reserved-bit/instruction-fetch/etc.
     * faults; demand-zero never repairs a protection fault. */
    if (!(x86_error&4u) || (x86_error&1u) || (x86_error&~6u)) {
        ++r->denied;
        return PORT_VMEM_PROTECTION;
    }
    uint32_t n=(va-r->start)/PORT_PAGE_BYTES;
    uint32_t pte=(r->start/PORT_PAGE_BYTES)+n;
    if (r->frames[n] || pt[pte]) {
        ++r->denied;
        return PORT_VMEM_PROTECTION;
    }
    uint32_t physical=port_phys_alloc(pool);
    if (!physical) return PORT_VMEM_OOM;
    /* Zero while physical memory is still supervisor-only. The guest's
     * identity map is valid for this reserved 1-3 MiB pool. */
    zero_page(physical,ctx);
    r->frames[n]=physical;
    pt[pte]=physical|7u;
    ++r->faults;
    return PORT_VMEM_HANDLED;
}

int port_vmem_release(struct port_vmem_region *r,
                      struct port_phys_pool *pool,
                      uint32_t pt[PORT_PMAP_ENTRIES]) {
    if (!r || !pool || !pt || !r->count) return PORT_VMEM_INVALID;
    /* Validate all allocated pages before changing anything. */
    for (uint32_t i=0;i<r->count;i++) {
        uint32_t phys=r->frames[i];
        uint32_t actual=pt[r->start/PORT_PAGE_BYTES+i];
        /* IA-32 hardware sets PTE Accessed (bit 5) and Dirty (bit 6)
         * during the CPL3 guest's memory reads/writes. They are not
         * permission or frame changes. All other bits must still match. */
        const uint32_t cpu_ad_bits=(1u<<5)|(1u<<6);
        if (phys && (!port_phys_is_owned(pool,phys) ||
                     (actual & ~cpu_ad_bits)!=(phys|7u)))
            return PORT_VMEM_PROTECTION;
        if (phys&&port_phys_pin_count(pool,phys))return PORT_VMEM_LOCKED;
        if (!phys && actual!=0) return PORT_VMEM_PROTECTION;
    }
    for (uint32_t i=0;i<r->count;i++) {
        uint32_t phys=r->frames[i];
        uint32_t va=r->start+i*PORT_PAGE_BYTES;
        pt[va/PORT_PAGE_BYTES]=va|3u;
        if (phys) (void)port_phys_free(pool,phys);
        r->frames[i]=0;
    }
    r->count=0;
    return 0;
}
uint32_t port_vmem_committed(const struct port_vmem_region *r) {
    if (!r) return 0;
    uint32_t n=0;
    for (uint32_t i=0;i<r->count && i<PORT_VMEM_MAX_PAGES;i++)
        if (r->frames[i]) ++n;
    return n;
}
