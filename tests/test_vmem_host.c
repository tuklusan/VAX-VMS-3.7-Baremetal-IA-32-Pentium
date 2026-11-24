#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include "vms_vmem.h"
static uint32_t table0[PORT_PMAP_ENTRIES],table1[PORT_PMAP_ENTRIES];
static uint32_t zeroes,seen_phys[16];
static void record_zero(uint32_t phys,void *ctx) {
    (void)ctx;
    assert(phys>=PORT_PHYS_BASE);
    assert(zeroes<16);
    seen_phys[zeroes++]=phys;
}
static void setup(uint32_t *t) {
    for (unsigned i=0;i<PORT_PMAP_ENTRIES;i++) t[i]=(i<<12)|3u;
}
int main(void) {
    struct port_phys_pool pool;
    struct port_vmem_region a={0},b={0};
    port_phys_init(&pool); setup(table0); setup(table1);
    assert(port_vmem_reserve(&a,table0,0x310000u,2)==0);
    assert(port_vmem_reserve(&b,table1,0x310000u,2)==0);
    assert(table0[0x310]==0 && table1[0x310]==0);
    assert(port_vmem_fault(&a,&pool,table0,0x310456u,6,record_zero,0)==1);
    assert(port_vmem_fault(&b,&pool,table1,0x310000u,4,record_zero,0)==1);
    assert(port_vmem_fault(&a,&pool,table0,0x311fffU,4,record_zero,0)==1);
    assert(port_vmem_fault(&b,&pool,table1,0x311100u,6,record_zero,0)==1);
    assert(zeroes==4 && pool.available==PORT_PHYS_FRAMES-4);
    assert(table0[0x310]!=table1[0x310]);
    assert((table0[0x310]&7)==7 && (table0[0x311]&7)==7);
    assert(port_vmem_committed(&a)==2 && port_vmem_committed(&b)==2);
    assert(port_vmem_fault(&a,&pool,table0,0x310000u,7,record_zero,0)==PORT_VMEM_PROTECTION);
    assert(port_vmem_fault(&a,&pool,table0,0x310000u,0,record_zero,0)==PORT_VMEM_PROTECTION);
    assert(port_vmem_fault(&a,&pool,table0,0x10000u,6,record_zero,0)==PORT_VMEM_INVALID);
    assert(a.denied==2 && a.faults==2);
    assert(port_vmem_reserve(&a,table0,0x320000u,2)==PORT_VMEM_INVALID); /* live region cannot overwrite? */
    /* Real x86 CPUs set PTE accessed/dirty bits; emulator guest run #7
     * proved the old exact-match validation leaked four frames. */
    table0[0x310] |= 0x20u;  /* hardware read: accessed */
    table0[0x311] |= 0x60u;  /* hardware write: accessed and dirty */
    table1[0x310] |= 0x60u;
    table1[0x311] |= 0x20u;
    /* A real privilege downgrade is still forbidden and atomic. */
    table0[0x311] &= ~2u;
    assert(port_vmem_release(&a,&pool,table0)==PORT_VMEM_PROTECTION);
    assert(pool.available==PORT_PHYS_FRAMES-4 && a.count==2);
    table0[0x311] |= 2u;
    const uint32_t pinned=table0[0x310]&0xfffff000u;
    assert(port_phys_pin(&pool,pinned)==0);
    assert(port_vmem_release(&a,&pool,table0)==PORT_VMEM_LOCKED&&
           pool.available==PORT_PHYS_FRAMES-4&&a.count==2&&
           (table0[0x310]&0xfffff000u)==pinned);
    assert(port_phys_unpin(&pool,pinned)==0);
    assert(port_vmem_release(&a,&pool,table0)==0);
    assert(port_vmem_release(&b,&pool,table1)==0);
    assert(pool.available==PORT_PHYS_FRAMES && table0[0x310]==0x310003u);
    assert(port_vmem_release(&a,&pool,table0)==PORT_VMEM_INVALID);
    assert(port_vmem_reserve(&a,table0,0x3ff000u,2)==PORT_VMEM_INVALID);
    assert(port_vmem_reserve(&a,table0,0x0u,1)==PORT_VMEM_INVALID);
    printf("M48 VMem host PASS: demand-zero, IA-32 A/D, locked-frame release guard, reclaim\n");
    return 0;
}
