#include "vms_exe_np_pool.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

static uint32_t depth;
static uint32_t enter(void *unused){(void)unused;return depth++;}
static void leave(void *unused,uint32_t old){(void)unused;assert(depth==old+1u);depth=old;}
int main(void){
    static _Alignas(8) uint8_t arena[256];
    const struct port_v37_irp_pool_sync sync={0,enter,leave};
    struct port_exe_np_pool pool;
    assert(port_exe_np_pool_init(&pool,arena,sizeof arena,&sync)==PORT_EXE_NP_POOL_OK);
    void *a=0,*b=0,*c=0;uint32_t an=0,bn=0,cn=0;
    assert(port_exe_np_pool_alloc(&pool,17u,&a,&an)==PORT_EXE_NP_POOL_OK&&an==24u);
    assert(port_exe_np_pool_alloc(&pool,32u,&b,&bn)==PORT_EXE_NP_POOL_OK&&bn==32u);
    assert(port_exe_np_pool_alloc(&pool,65u,&c,&cn)==PORT_EXE_NP_POOL_OK&&cn==72u);
    assert((uintptr_t)b-(uintptr_t)a==24u&&(uintptr_t)c-(uintptr_t)b==32u);
    assert(port_exe_np_pool_check(&pool)==PORT_EXE_NP_POOL_OK&&pool.allocations==3u);
    assert(port_exe_np_pool_free(&pool,b,bn)==PORT_EXE_NP_POOL_OK);
    assert(port_exe_np_pool_free(&pool,a,an)==PORT_EXE_NP_POOL_OK);
    assert(port_exe_np_pool_free(&pool,a,an)==PORT_EXE_NP_POOL_DUP);
    assert(port_exe_np_pool_free(&pool,c,cn)==PORT_EXE_NP_POOL_OK);
    assert(pool.allocations==0u&&pool.head_offset==0u&&
           ((struct port_exe_np_free *)arena)->bytes==sizeof arena&&
           ((struct port_exe_np_free *)arena)->next_offset==UINT32_MAX);
    assert(port_exe_np_pool_check(&pool)==PORT_EXE_NP_POOL_OK&&depth==0u);
    assert(port_exe_np_pool_alloc(&pool,257u,&a,&an)==PORT_EXE_NP_POOL_EMPTY);
    assert(port_exe_np_pool_init(&pool,arena+1u,sizeof arena-1u,&sync)==PORT_EXE_NP_POOL_BAD);
    puts("MEMORYALC first-fit split, sorted coalesce, bounds, duplicate free and IPL restore PASS");
    return 0;
}
