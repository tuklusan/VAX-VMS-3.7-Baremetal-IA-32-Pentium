#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "vms_phys.h"
int main(void) {
    struct port_phys_pool pool;
    uint32_t addr[PORT_PHYS_FRAMES];
    port_phys_init(&pool);
    assert(port_phys_available(&pool)==PORT_PHYS_FRAMES);
    for(unsigned i=0;i<PORT_PHYS_FRAMES;i++) {
        addr[i]=port_phys_alloc(&pool);
        assert(addr[i]==PORT_PHYS_BASE+i*PORT_PHYS_FRAME_BYTES);
        assert(port_phys_is_owned(&pool,addr[i]));
    }
    assert(port_phys_alloc(&pool)==0 && pool.high_water==PORT_PHYS_FRAMES);
    assert(port_phys_free(&pool,addr[0]+3)<0);
    assert(port_phys_free(&pool,addr[0])==0);
    assert(port_phys_free(&pool,addr[0])<0);
    assert(port_phys_alloc(&pool)==addr[0]);
    for(unsigned i=0;i<PORT_PHYS_FRAMES;i++)
        assert(port_phys_free(&pool,addr[i])==0);
    assert(port_phys_available(&pool)==PORT_PHYS_FRAMES);
    assert(port_phys_free(&pool,PORT_PHYS_BASE-4096)<0);
    puts("Physical frame allocator PASS: exhaust, recycle, double-free, bounds");
    return 0;
}
