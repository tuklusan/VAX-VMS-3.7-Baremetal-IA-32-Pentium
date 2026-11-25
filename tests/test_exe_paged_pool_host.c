#include "vms_exe_paged_pool.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

static uint32_t held;
static uint32_t raise_sync(void *unused){(void)unused;return held++;}
static void restore_ipl(void *unused,uint32_t previous){
    (void)unused;assert(held==previous+1u);held=previous;
}

int main(void){
    static _Alignas(16) uint8_t memory[256];
    static _Alignas(16) uint8_t misaligned[272];
    const struct port_v37_irp_pool_sync sync={0,raise_sync,restore_ipl};
    struct port_exe_paged_pool pool;
    assert(port_exe_paged_pool_init(&pool,memory,sizeof memory,&sync)==
           PORT_EXE_PAGED_OK);
    assert(port_exe_paged_pool_init(&pool,misaligned+8,sizeof misaligned-8,&sync)==
           PORT_EXE_PAGED_BAD);
    void *a=0,*b=0;uint32_t an=0,bn=0;
    assert(port_exe_paged_alloc(&pool,17u,&a,&an)==PORT_EXE_PAGED_OK&&an==32u);
    assert(port_exe_paged_alloc(&pool,32u,&b,&bn)==PORT_EXE_PAGED_OK&&bn==32u);
    assert(((uintptr_t)a&15u)==0u&&((uintptr_t)b&15u)==0u&&
           (uintptr_t)b-(uintptr_t)a==32u);
    assert(port_exe_paged_alloc(&pool,193u,&b,&bn)==PORT_EXE_PAGED_EMPTY);
    assert(port_exe_paged_free(&pool,a,17u)==PORT_EXE_PAGED_OK);
    assert(port_exe_paged_free(&pool,a,17u)==PORT_EXE_PAGED_DUP);
    assert(port_exe_paged_free(&pool,b,bn)==PORT_EXE_PAGED_OK);
    assert(pool.free_blocks.allocations==0u&&pool.free_blocks.head_offset==0u&&
           ((struct port_exe_np_free *)memory)->bytes==sizeof memory&&held==0u);
    puts("MEMORYALC paged allocation: 16-byte size, first fit, exhaustion, return/coalesce, IPL PASS");
    return 0;
}
