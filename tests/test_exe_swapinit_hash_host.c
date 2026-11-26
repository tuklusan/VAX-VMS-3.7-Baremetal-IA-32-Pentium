#include "vms_exe_swapinit_hash.h"
#include "vms_ini_log_templates.h"
#include "vms_log_system.h"
#include "vms_v37_dyn_tags.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

static uint32_t get16(const uint8_t *p){return (uint32_t)p[0]|((uint32_t)p[1]<<8);}
static uint32_t get32(const uint8_t *p){
    return (uint32_t)p[0]|((uint32_t)p[1]<<8)|
           ((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);
}
static void put32(uint8_t *p,uint32_t v){
    for(unsigned i=0u;i<4u;i++)p[i]=(uint8_t)(v>>(8u*i));
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
    void *owned=0;uint32_t owned_bytes=0u;
    assert(port_exe_paged_alloc(&pool,48u,&owned,&owned_bytes)==
           PORT_EXE_PAGED_OK);
    tables.owned_records[0]=owned;tables.owned_record_bytes[0]=owned_bytes;
    assert(port_exe_swapinit_hash_release(&pool,&tables)==
           PORT_EXE_SWAPINIT_HASH_OK);

    uint8_t disk[PORT_INI_LOG_TEMPLATE_BYTES];
    uint8_t sysdev[PORT_INI_LOG_TEMPLATE_BYTES];
    port_ini_log_templates(disk,sysdev);
    const uint32_t one_bucket[3]={1u,1u,1u};
    assert(port_exe_swapinit_hash_init(&pool,one_bucket,&tables)==
           PORT_EXE_SWAPINIT_HASH_OK);
    tables.table[0].record_address=0x00100000u;
    uint8_t bad_name[PORT_INI_LOG_TEMPLATE_BYTES];
    for(unsigned i=0u;i<sizeof bad_name;i++)bad_name[i]=disk[i];
    bad_name[21u]='X';
    assert(port_exe_swapinit_insert_system_record(&tables,bad_name,
        0x00200200u)==PORT_EXE_SWAPINIT_HASH_BAD);
    struct port_exe_swapinit_name_record source_names[2]={
        {sysdev,0x00200100u},{disk,0x00200000u}};
    /* Source LOG$INSLOGN order is SYS$DISK then SYS$SYSDEVICE. Exercise the
     * opposite order to verify the sorted insertion point and back-links. */
    assert(port_exe_swapinit_insert_system_record(&tables,
        source_names[0].bytes,source_names[0].address)==
           PORT_EXE_SWAPINIT_HASH_OK);
    assert(port_exe_swapinit_insert_system_record(&tables,
        source_names[1].bytes,source_names[1].address)==
           PORT_EXE_SWAPINIT_HASH_OK);
    const uint32_t bucket_address=tables.table[0].record_address+12u;
    assert(get32(tables.table[0].buckets)==source_names[1].address&&
           get32(disk)==source_names[0].address&&
           get32(disk+4u)==bucket_address&&get32(sysdev)==0u&&
           get32(sysdev+4u)==source_names[1].address&&
           tables.table[0].linked_count==2u);
    uint32_t found=0u;
    static const uint8_t sysdisk_name[8]={
        'S','Y','S','$','D','I','S','K'};
    static const uint8_t sysdevice_name[13]={
        'S','Y','S','$','S','Y','S','D','E','V','I','C','E'};
    static const uint8_t missing_name[12]={
        'S','Y','S','$','N','O','T','F','O','U','N','D'};
    assert(port_exe_swapinit_search_system(&tables,sysdisk_name,
        sizeof sysdisk_name,&found)==PORT_LOG_OK&&
        found==source_names[1].address);
    assert(port_exe_swapinit_search_system(&tables,sysdevice_name,
        sizeof sysdevice_name,&found)==PORT_LOG_OK&&
        found==source_names[0].address);
    found=0xdeadbeefu;
    assert(port_exe_swapinit_search_system(&tables,missing_name,
        sizeof missing_name,&found)==PORT_LOG_NOTRAN&&found==0xdeadbeefu);
    const uint32_t first_bucket=get32(tables.table[0].buckets);
    put32(tables.table[0].buckets,0x00ffffffu);
    assert(port_exe_swapinit_search_system(&tables,sysdisk_name,
        sizeof sysdisk_name,&found)==PORT_LOG_CYCLE&&found==0xdeadbeefu);
    put32(tables.table[0].buckets,first_bucket);
    assert(port_exe_swapinit_insert_system_record(&tables,disk,
        source_names[1].address)==PORT_EXE_SWAPINIT_HASH_DUP);
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
    puts("SWAPPER EXE$SWAPINIT RSHT build, bounded LOG$SEARCHLOG and rollback PASS");
    return 0;
}
