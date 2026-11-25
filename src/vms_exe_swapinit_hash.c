#include "vms_exe_swapinit_hash.h"
#include "vms_v37_dyn_tags.h"

static void put16(uint8_t *p,uint16_t x){
    p[0]=(uint8_t)x;p[1]=(uint8_t)(x>>8);
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
        t->allocated_bytes=allocated;
        for(uint32_t j=0u;j<allocated;j++)t->record[j]=0u;
        /* SWAPPER.MAR 30$: clear queue links, then write size and RSHT tag. */
        put16(t->record+8u,(uint16_t)allocated);
        t->record[10u]=PORT_V37_DYN_RSHT;
    }
    *out=candidate;
    return PORT_EXE_SWAPINIT_HASH_OK;
}

uint32_t port_exe_swapinit_hash_release(struct port_exe_paged_pool *pool,
    struct port_exe_swapinit_hash_set *set){
    if(!pool||!set)return PORT_EXE_SWAPINIT_HASH_BAD;
    const uint32_t status=release_candidate(pool,set);
    if(status==PORT_EXE_SWAPINIT_HASH_OK)*set=(struct port_exe_swapinit_hash_set){0};
    return status;
}
