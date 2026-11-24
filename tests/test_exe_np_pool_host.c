#include "vms_exe_np_pool.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

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
    static _Alignas(16) uint8_t irp_mem[512],lrp_mem[512],srp_mem[512];
    struct port_ini_packet_area areas[3]={
        {irp_mem,0x1000u,sizeof irp_mem,1u,80u,64u,0x8000u},
        {lrp_mem,0x2000u,sizeof lrp_mem,2u,160u,136u,0x8008u},
        {srp_mem,0x3000u,sizeof srp_mem,2u,48u,24u,0x8010u}};
    struct port_ini_packet_list lists[3];
    for(unsigned i=0;i<3u;i++)
        assert(port_ini_packet_init(&areas[i],&lists[i])==PORT_INI_PACKETS_OK);
    struct port_exe_np_allocator allocator={
        .irps=&lists[0],.lrps=&lists[1],.srps=&lists[2],
        .variable=&pool,.sync=&sync};
    struct port_exe_np_allocation allocation;
    assert(port_exe_np_allocate(&allocator,48u,7u,&allocation)==PORT_EXE_NP_POOL_OK&&
           allocation.source==PORT_EXE_NP_FROM_SRP&&allocation.bytes==48u);
    memset(allocation.address,0xa5u,allocation.bytes);
    assert(port_exe_np_deallocate(&allocator,allocation.address,48u,7u)==
           PORT_EXE_NP_POOL_OK&&lists[2].free_count==2u);
    assert(port_exe_np_allocate(&allocator,80u,10u,&allocation)==PORT_EXE_NP_POOL_OK&&
           allocation.source==PORT_EXE_NP_FROM_IRP&&allocation.bytes==80u);
    memset(allocation.address,0x5au,allocation.bytes);
    assert(port_exe_np_deallocate(&allocator,allocation.address,80u,10u)==
           PORT_EXE_NP_POOL_OK&&lists[0].free_count==1u);
    assert(port_exe_np_allocate(&allocator,160u,18u,&allocation)==PORT_EXE_NP_POOL_OK&&
           allocation.source==PORT_EXE_NP_FROM_LRP&&allocation.bytes==160u);
    memset(allocation.address,0x3cu,allocation.bytes);
    assert(port_exe_np_deallocate(&allocator,allocation.address,160u,18u)==
           PORT_EXE_NP_POOL_OK&&lists[1].free_count==2u);
    assert(port_exe_np_allocate(&allocator,104u,7u,&allocation)==PORT_EXE_NP_POOL_OK&&
           allocation.source==PORT_EXE_NP_FROM_VARIABLE&&allocation.bytes==104u);
    assert(port_exe_np_deallocate(&allocator,allocation.address,104u,7u)==
           PORT_EXE_NP_POOL_OK&&pool.allocations==0u);
    puts("MEMORYALC first-fit split, sorted coalesce, bounds, duplicate free and IPL restore PASS");
    return 0;
}
