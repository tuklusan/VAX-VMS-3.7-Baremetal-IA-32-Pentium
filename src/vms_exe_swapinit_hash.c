#include "vms_exe_swapinit_hash.h"
#include "vms_v37_dyn_tags.h"
#include "vms_log_system.h"

static void put16(uint8_t *p,uint16_t x){
    p[0]=(uint8_t)x;p[1]=(uint8_t)(x>>8);
}
static uint32_t get32(const uint8_t *p){
    return (uint32_t)p[0]|((uint32_t)p[1]<<8)|
           ((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);
}
static void put32(uint8_t *p,uint32_t x){
    p[0]=(uint8_t)x;p[1]=(uint8_t)(x>>8);
    p[2]=(uint8_t)(x>>16);p[3]=(uint8_t)(x>>24);
}
static uint32_t table_geometry(uint32_t requested,uint32_t *count,
                               uint8_t *bits){
    if(!requested||requested>16383u||!count||!bits)
        return PORT_EXE_SWAPINIT_HASH_BAD;
    uint32_t n=1u;uint8_t b=0u;
    while(n<requested){n<<=1u;++b;}
    *count=n;*bits=b;
    return PORT_EXE_SWAPINIT_HASH_OK;
}

static uint32_t release_candidate(struct port_exe_paged_pool *pool,
                                  struct port_exe_swapinit_hash_set *set){
    for(unsigned i=PORT_EXE_SWAPINIT_HASH_TABLES;i>0u;i--){
        struct port_exe_swapinit_hash_table *t=&set->table[i-1u];
        if(!t->record)continue;
        const uint32_t rc=port_exe_paged_free(pool,t->record,
                                              t->allocated_bytes);
        if(rc!=PORT_EXE_PAGED_OK)return PORT_EXE_SWAPINIT_HASH_CORRUPT;
        t->record=0;t->buckets=0;t->allocated_bytes=0u;
    }
    return PORT_EXE_SWAPINIT_HASH_OK;
}

uint32_t port_exe_swapinit_hash_init(struct port_exe_paged_pool *pool,
    const uint32_t requested[PORT_EXE_SWAPINIT_HASH_TABLES],
    struct port_exe_swapinit_hash_set *out){
    if(!pool||!requested||!out)return PORT_EXE_SWAPINIT_HASH_BAD;
    for(unsigned i=0u;i<PORT_EXE_SWAPINIT_INITIAL_NAMES;i++)
        if(out->owned_records[i]||out->owned_record_bytes[i])
            return PORT_EXE_SWAPINIT_HASH_BAD;
    for(unsigned i=0u;i<PORT_EXE_SWAPINIT_HASH_TABLES;i++)
        if(out->table[i].record||out->table[i].allocated_bytes)
            return PORT_EXE_SWAPINIT_HASH_BAD;
    struct port_exe_swapinit_hash_set candidate={0};
    for(unsigned i=0u;i<PORT_EXE_SWAPINIT_HASH_TABLES;i++){
        struct port_exe_swapinit_hash_table *t=&candidate.table[i];
        if(table_geometry(requested[i],&t->entry_count,&t->entry_log2)!=
           PORT_EXE_SWAPINIT_HASH_OK){
            (void)release_candidate(pool,&candidate);
            return PORT_EXE_SWAPINIT_HASH_BAD;
        }
        t->requested_entries=requested[i];
        void *record=0;uint32_t allocated=0u;
        /* SWAPPER allocates SYSTEM and GROUP tables here. PROCESS size and
         * exponent are normalized now, but each process owns its own table. */
        if(i>=2u)continue;
        /* LOG$GL_HTBLSIZ counts longword links, and VAX indexed longword
         * addressing in SWAPPER 30$ scales R1 by four. */
        const uint32_t bytes=PORT_EXE_SWAPINIT_RSHT_HEADER_BYTES+
                             t->entry_count*4u;
        const uint32_t status=port_exe_paged_alloc(pool,bytes,&record,
                                                    &allocated);
        if(status!=PORT_EXE_PAGED_OK){
            const uint32_t rollback=release_candidate(pool,&candidate);
            return rollback==PORT_EXE_SWAPINIT_HASH_OK?
                PORT_EXE_SWAPINIT_HASH_NOMEM:rollback;
        }
        t->record=record;t->buckets=t->record+
                                      PORT_EXE_SWAPINIT_RSHT_HEADER_BYTES;
        t->record_address=(uintptr_t)record<=UINT32_MAX?
            (uint32_t)(uintptr_t)record:0u;
        t->allocated_bytes=allocated;
        for(uint32_t j=0u;j<allocated;j++)t->record[j]=0u;
        /* SWAPPER.MAR 30$: clear queue links, then write size and RSHT tag. */
        put16(t->record+8u,(uint16_t)allocated);
        t->record[10u]=PORT_V37_DYN_RSHT;
    }
    *out=candidate;
    return PORT_EXE_SWAPINIT_HASH_OK;
}

static struct port_exe_swapinit_name_record *find_name(
    struct port_exe_swapinit_hash_table *table,uint32_t address){
    for(unsigned i=0u;i<table->linked_count;i++)
        if(table->linked_names[i].address==address)
            return &table->linked_names[i];
    return 0;
}

uint32_t port_exe_swapinit_insert_system_record(
    struct port_exe_swapinit_hash_set *set,uint8_t *record,uint32_t address){
    if(!set||!record||!address)return PORT_EXE_SWAPINIT_HASH_BAD;
    struct port_exe_swapinit_hash_table *table=&set->table[0];
    if(!table->record||!table->buckets||!table->record_address||
       table->entry_log2>PORT_LOG_MAX_BITS||
       table->entry_count!=(1u<<table->entry_log2)||
       (uint64_t)table->record_address+PORT_EXE_SWAPINIT_RSHT_HEADER_BYTES+
           (uint64_t)table->entry_count*4u>UINT32_MAX+UINT64_C(1))
        return PORT_EXE_SWAPINIT_HASH_BAD;
    const uint8_t name_bytes=record[20u];
    const uint8_t *name=record+21u;
    if(!name_bytes||port_log_record_valid(record,PORT_LOG_REC_LEN,
          (const char *)name,name_bytes)!=PORT_LOG_OK)
        return PORT_EXE_SWAPINIT_HASH_BAD;
    static const uint8_t disk_name[8]={'S','Y','S','$','D','I','S','K'};
    static const uint8_t sysdev_name[13]={
        'S','Y','S','$','S','Y','S','D','E','V','I','C','E'};
    if(!((name_bytes==sizeof disk_name&&
          port_log_name_compare(name,name_bytes,disk_name,sizeof disk_name)==1u)||
         (name_bytes==sizeof sysdev_name&&
          port_log_name_compare(name,name_bytes,sysdev_name,
                                sizeof sysdev_name)==1u)))
        return PORT_EXE_SWAPINIT_HASH_BAD;
    for(unsigned i=0u;i<table->linked_count;i++){
        struct port_exe_swapinit_name_record *old=&table->linked_names[i];
        if(old->address==address)return PORT_EXE_SWAPINIT_HASH_DUP;
        if(port_log_name_compare(old->bytes+21u,old->bytes[20u],
                                 name,name_bytes)==1u)
            return PORT_EXE_SWAPINIT_HASH_DUP;
    }
    if(table->linked_count>=PORT_EXE_SWAPINIT_INITIAL_NAMES)
        return PORT_EXE_SWAPINIT_HASH_BAD;
    const uint32_t bucket=port_log_hash(name,name_bytes,table->entry_log2);
    if(bucket>=table->entry_count)return PORT_EXE_SWAPINIT_HASH_CORRUPT;
    uint8_t *link=table->buckets+bucket*4u;
    uint32_t link_address=table->record_address+
        PORT_EXE_SWAPINIT_RSHT_HEADER_BYTES+bucket*4u;
    uint32_t next=get32(link);
    for(unsigned walked=0u;next;walked++){
        if(walked>=table->linked_count)return PORT_EXE_SWAPINIT_HASH_CORRUPT;
        struct port_exe_swapinit_name_record *old=find_name(table,next);
        if(!old)return PORT_EXE_SWAPINIT_HASH_CORRUPT;
        const uint32_t order=port_log_name_compare(old->bytes+21u,
            old->bytes[20u],name,name_bytes);
        if(order==1u)return PORT_EXE_SWAPINIT_HASH_DUP;
        if(order==2u)break;
        link=old->bytes;link_address=old->address;
        next=get32(old->bytes);
    }
    put32(record,next);
    put32(record+4u,link_address);
    if(next){
        struct port_exe_swapinit_name_record *old=find_name(table,next);
        if(!old)return PORT_EXE_SWAPINIT_HASH_CORRUPT;
        put32(old->bytes+4u,address);
    }
    put32(link,address);
    table->linked_names[table->linked_count++]=
        (struct port_exe_swapinit_name_record){record,address};
    return PORT_EXE_SWAPINIT_HASH_OK;
}

uint32_t port_exe_swapinit_hash_release(struct port_exe_paged_pool *pool,
    struct port_exe_swapinit_hash_set *set){
    if(!pool||!set)return PORT_EXE_SWAPINIT_HASH_BAD;
    for(unsigned i=PORT_EXE_SWAPINIT_INITIAL_NAMES;i>0u;i--){
        const unsigned n=i-1u;
        if(!set->owned_records[n])continue;
        const uint32_t rc=port_exe_paged_free(pool,set->owned_records[n],
            set->owned_record_bytes[n]);
        if(rc!=PORT_EXE_PAGED_OK)return PORT_EXE_SWAPINIT_HASH_CORRUPT;
        set->owned_records[n]=0;set->owned_record_bytes[n]=0u;
    }
    const uint32_t status=release_candidate(pool,set);
    if(status==PORT_EXE_SWAPINIT_HASH_OK)*set=(struct port_exe_swapinit_hash_set){0};
    return status;
}
