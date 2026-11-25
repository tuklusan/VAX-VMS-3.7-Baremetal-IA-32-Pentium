#include "vms_exe_swapinit_hash.h"
#include "vms_v37_dyn_tags.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

static uint32_t get16(const uint8_t *p){return (uint32_t)p[0]|((uint32_t)p[1]<<8);}
static uint32_t get32(const uint8_t *p){
    return (uint32_t)p[0]|((uint32_t)p[1]<<8)|
           ((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);
}
static uint32_t held_ipl;
static uint32_t raise_sync(void *unused){(void)unused;return held_ipl++;}
static void restore_ipl(void *unused,uint32_t previous){
    (void)unused;assert(held_ipl==previous+1u);held_ipl=previous;
}

int main(void){
    static _Alignas(16) uint8_t memory[32768];
    const uint32_t requested[3]={128u,32u,128u};
    const struct port_v37_irp_pool_sync sync={0,raise_sync,restore_ipl};
    struct port_exe_paged_pool pool;
    assert(port_exe_paged_pool_init(&pool,memory,sizeof memory,&sync)==
           PORT_EXE_PAGED_OK);
    struct port_exe_swapinit_hash_set tables={0};
    assert(port_exe_swapinit_hash_init(&pool,requested,&tables)==
           PORT_EXE_SWAPINIT_HASH_OK);
    void *const system_record=tables.table[0].record;
    assert(port_exe_swapinit_hash_init(&pool,requested,&tables)==
           PORT_EXE_SWAPINIT_HASH_BAD&&
           tables.table[0].record==system_record&&
           pool.free_blocks.allocations==2u);
    const uint32_t counts[3]={128u,32u,128u};
    const uint8_t bits[3]={7u,5u,7u};
    for(unsigned i=0;i<3u;i++){
        const struct port_exe_swapinit_hash_table *t=&tables.table[i];
        assert(t->requested_entries==requested[i]&&
               t->entry_count==counts[i]&&t->entry_log2==bits[i]);
        if(i<2u){
            assert(t->record&&t->buckets==t->record+12u&&
                   t->allocated_bytes==((12u+counts[i]*4u+15u)&~15u)&&
                   get32(t->record)==0u&&get32(t->record+4u)==0u&&
                   get16(t->record+8u)==t->allocated_bytes&&
                   t->record[10u]==PORT_V37_DYN_RSHT&&t->record[11u]==0u);
            for(uint32_t j=0u;j<counts[i]*4u;j++)assert(t->buckets[j]==0u);
        }else assert(!t->record&&!t->buckets&&!t->allocated_bytes);
    }
    assert(port_exe_swapinit_hash_release(&pool,&tables)==
           PORT_EXE_SWAPINIT_HASH_OK);
    assert(!tables.table[0].record&&!tables.table[1].record&&
           !tables.table[2].record&&pool.free_blocks.allocations==0u&&
           pool.free_blocks.head_offset==0u);
    const uint32_t edge[3]={1u,129u,1u};
    assert(port_exe_swapinit_hash_init(&pool,edge,&tables)==
           PORT_EXE_SWAPINIT_HASH_OK&&
           tables.table[0].entry_count==1u&&tables.table[0].entry_log2==0u&&
           tables.table[1].entry_count==256u&&tables.table[1].entry_log2==8u&&
           tables.table[1].allocated_bytes==1040u&&
           tables.table[2].entry_count==1u&&tables.table[2].entry_log2==0u&&
           !tables.table[2].record);
    assert(port_exe_swapinit_hash_release(&pool,&tables)==
           PORT_EXE_SWAPINIT_HASH_OK);

    static _Alignas(16) uint8_t small[560];
    struct port_exe_paged_pool small_pool;
    assert(port_exe_paged_pool_init(&small_pool,small,sizeof small,&sync)==
           PORT_EXE_PAGED_OK);
    struct port_exe_swapinit_hash_set untouched={.table={{.entry_count=77u}}};
    const struct port_exe_swapinit_hash_set before=untouched;
    assert(port_exe_swapinit_hash_init(&small_pool,requested,&untouched)==
           PORT_EXE_SWAPINIT_HASH_NOMEM);
    assert(untouched.table[0].entry_count==before.table[0].entry_count&&
           small_pool.free_blocks.allocations==0u&&
           small_pool.free_blocks.head_offset==0u);
    const uint32_t invalid[3]={1u,0u,2u};
    assert(port_exe_swapinit_hash_init(&pool,invalid,&tables)==
           PORT_EXE_SWAPINIT_HASH_BAD);
    puts("SWAPPER EXE$SWAPINIT RSHT geometry, zeroed buckets, DYN headers, rollback/coalescing PASS");
    return 0;
}
