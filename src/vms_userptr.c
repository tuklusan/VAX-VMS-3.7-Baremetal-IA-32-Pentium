#include "vms_userptr.h"
int port_user_range_valid(const struct port_user_map *m,
                          uint32_t base,uint32_t len,int writing) {
    if(!m||!m->pde||!m->resolve||!len||len>PORT_USER_RANGE_MAX||
       base<4096u||base>UINT32_MAX-(len-1u))return PORT_USER_BAD;
    uint32_t last=base+len-1u;
    /* Bound is <= 8 KiB, at most three page translations. */
    uint32_t address=base;
    for(;;){
        uint32_t dir=m->pde[(address>>22)&1023u];
        if((dir&5u)!=5u||(dir&0x80u)||(writing&&!(dir&2u)))return PORT_USER_BAD;
        const uint32_t *pte=m->resolve(m->context,dir&0xfffff000u);
        if(!pte)return PORT_USER_BAD;
        uint32_t entry=pte[(address>>12)&1023u];
        if((entry&5u)!=5u||(writing&&!(entry&2u)))return PORT_USER_BAD;
        if((address>>12)==(last>>12))return PORT_USER_OK;
        address=(address&0xfffff000u)+4096u;
    }
}

int port_user_io_range_valid(const struct port_user_map *m,uint32_t base,
                             uint32_t len,int writing){
    if(!m||!m->pde||!m->resolve||!len||len>UINT16_MAX||base<4096u||
       base>UINT32_MAX-(len-1u))return PORT_USER_BAD;
    const uint32_t last=base+len-1u;
    uint32_t address=base;
    for(;;){
        const uint32_t dir=m->pde[(address>>22)&1023u];
        if((dir&5u)!=5u||(dir&0x80u)||(writing&&!(dir&2u)))
            return PORT_USER_BAD;
        const uint32_t *table=m->resolve(m->context,dir&0xfffff000u);
        if(!table)return PORT_USER_BAD;
        const uint32_t pte=table[(address>>12)&1023u];
        if((pte&5u)!=5u||(writing&&!(pte&2u)))return PORT_USER_BAD;
        if((address>>12)==(last>>12))return PORT_USER_OK;
        address=(address&0xfffff000u)+4096u;
    }
}

int port_user_pin_io_range(const struct port_user_map *m,uint32_t base,
                           uint32_t bytes,int writing,
                           port_phys_page_resolver physical,void *physical_ctx,
                           port_user_page_pin pin,port_user_page_unpin unpin,
                           void *lock_ctx,struct port_user_page_locks *locks){
    if(!locks||locks->count||!physical||!pin||!unpin||
       port_user_io_range_valid(m,base,bytes,writing))return PORT_USER_BAD;
    const uint32_t last=base+bytes-1u;
    struct port_user_page_locks acquired={{0},0,unpin,lock_ctx};
    for(uint32_t at=base;;){
        const uint32_t dir=m->pde[(at>>22)&1023u];
        const uint32_t *table=m->resolve(m->context,dir&0xfffff000u);
        if(!table||acquired.count==PORT_USER_PIN_MAX_PAGES)goto rollback;
        const uint32_t pte=table[(at>>12)&1023u];
        const uint32_t frame=pte&0xfffff000u;
        /* A valid PTE whose physical frame cannot be resolved is not
         * lockable; do not retain a partial pin set. */
        if(!physical(physical_ctx,frame)||pin(lock_ctx,frame))goto rollback;
        acquired.physical[acquired.count++]=frame;
        if((at>>12)==(last>>12))break;
        at=(at&0xfffff000u)+4096u;
    }
    *locks=acquired;
    return PORT_USER_OK;
rollback:
    while(acquired.count)unpin(lock_ctx,acquired.physical[--acquired.count]);
    return PORT_USER_BAD;
}

void port_user_unpin_io_range(struct port_user_page_locks *locks){
    if(!locks||!locks->release||locks->count>PORT_USER_PIN_MAX_PAGES)return;
    while(locks->count)locks->release(locks->release_context,
                                      locks->physical[--locks->count]);
    for(unsigned i=0;i<PORT_USER_PIN_MAX_PAGES;i++)locks->physical[i]=0;
    locks->release=0;locks->release_context=0;
}

/* The output contract is all-or-nothing for unmapped spans. Retain the
 * validated physical page aliases so a page-crossing copy never assumes
 * that consecutive user virtual pages are physically adjacent. */
int port_user_copy_to(const struct port_user_map *m, uint32_t dest,
                      const void *source, uint32_t bytes,
                      port_phys_page_resolver physical, void *context) {
    if(!source||!physical||port_user_range_valid(m,dest,bytes,1))
        return PORT_USER_BAD;
    struct copy_span { uint8_t *where; uint32_t count; } spans[3];
    uint32_t remaining=bytes, current=dest;
    unsigned count=0;
    while(remaining){
        uint32_t dir=m->pde[(current>>22)&1023u];
        const uint32_t *table=m->resolve(m->context,dir&0xfffff000u);
        if(!table || count>=3u)return PORT_USER_BAD;
        uint32_t pte=table[(current>>12)&1023u];
        uint8_t *physical_page=physical(context,pte&0xfffff000u);
        if(!physical_page)return PORT_USER_BAD;
        uint32_t offset=current&4095u;
        uint32_t n=4096u-offset;
        if(n>remaining)n=remaining;
        spans[count].where=physical_page+offset;
        spans[count].count=n;
        ++count;
        current+=n;remaining-=n;
    }
    const uint8_t *in=(const uint8_t *)source;
    for(unsigned i=0;i<count;i++){
        for(uint32_t j=0;j<spans[i].count;j++)spans[i].where[j]=*in++;
    }
    return PORT_USER_OK;
}

/* Input copy: translated physical alias, never a CPL0 dereference of an
 * unchecked user pointer.  A full two-pass preflight provides the same
 * no-partial-copy contract as copy_to for missing physical frames. */
int port_user_copy_from(const struct port_user_map *m, void *destination,
                        uint32_t source, uint32_t bytes,
                        port_phys_page_resolver physical, void *context) {
    if(!destination || !physical || port_user_range_valid(m,source,bytes,0))
        return PORT_USER_BAD;
    struct read_span { const uint8_t *where; uint32_t count; } spans[3];
    uint32_t current=source, remaining=bytes;
    unsigned count=0;
    while(remaining) {
        uint32_t dir=m->pde[(current>>22)&1023u];
        const uint32_t *table=m->resolve(m->context,dir&0xfffff000u);
        if(!table || count>=3u)return PORT_USER_BAD;
        uint32_t entry=table[(current>>12)&1023u];
        const uint8_t *page=physical(context,entry&0xfffff000u);
        if(!page)return PORT_USER_BAD;
        uint32_t offset=current&4095u;
        uint32_t take=4096u-offset;
        if(take>remaining)take=remaining;
        spans[count].where=page+offset;
        spans[count].count=take;
        ++count;
        current+=take;remaining-=take;
    }
    uint8_t *out=(uint8_t *)destination;
    for(unsigned i=0;i<count;i++)
        for(uint32_t j=0;j<spans[i].count;j++)*out++=spans[i].where[j];
    return PORT_USER_OK;
}

/* Transactional scatter output.  The buffer+IOSB pair is committed only
 * when all PTEs/physical frames exist; reject even virtual aliases that
 * overlap in *physical* memory. Never dereference raw user virtual pointers. */
int port_user_copyv_to(const struct port_user_map *m,
                       const struct port_user_output *outputs,
                       unsigned count, port_phys_page_resolver physical,
                       void *context) {
    if(!m || !outputs || !physical || !count || count>PORT_USER_OUTPUT_MAX)
        return PORT_USER_BAD;
    struct output_span {
        uint8_t *to;
        const uint8_t *from;
        uint32_t size;
    } spans[3u*PORT_USER_OUTPUT_MAX];
    unsigned total=0;
    for(unsigned i=0;i<count;i++) {
        const struct port_user_output *o=&outputs[i];
        if(!o->source || port_user_range_valid(m,o->destination,o->bytes,1))
            return PORT_USER_BAD;
        uint32_t left=o->bytes,cur=o->destination;
        const uint8_t *source=(const uint8_t *)o->source;
        while(left) {
            if(total>=3u*PORT_USER_OUTPUT_MAX)return PORT_USER_BAD;
            const uint32_t dir=m->pde[(cur>>22)&1023u];
            const uint32_t *table=m->resolve(m->context,dir&0xfffff000u);
            if(!table)return PORT_USER_BAD;
            const uint32_t entry=table[(cur>>12)&1023u];
            uint8_t *page=physical(context,entry&0xfffff000u);
            if(!page)return PORT_USER_BAD;
            uint32_t offset=cur&4095u;
            uint32_t n=4096u-offset;
            if(n>left)n=left;
            spans[total].to=page+offset;
            spans[total].from=source;
            spans[total].size=n;
            ++total;
            cur+=n;source+=n;left-=n;
        }
    }
    /* Detect physical overlaps regardless of aliases or virtual address. */
    for(unsigned i=0;i<total;i++) {
        uintptr_t a=(uintptr_t)spans[i].to;
        for(unsigned j=i+1;j<total;j++) {
            uintptr_t b=(uintptr_t)spans[j].to;
            if(a<=b ? (b-a)<spans[i].size : (a-b)<spans[j].size)
                return PORT_USER_BAD;
        }
    }
    for(unsigned i=0;i<total;i++)
        for(uint32_t j=0;j<spans[i].size;j++)
            spans[i].to[j]=spans[i].from[j];
    return PORT_USER_OK;
}

/* Reject aliased output destinations BEFORE queueing disk I/O. This is
 * separate from the copyv commit check: users must not be allowed to
 * enqueue a request that can never commit because of aliasing. */
int port_user_outputs_disjoint(const struct port_user_map *m,
                               uint32_t first,uint32_t first_len,
                               uint32_t second,uint32_t second_len,
                               port_phys_page_resolver physical,void *context) {
    if(!m||!physical || port_user_range_valid(m,first,first_len,1) ||
       port_user_range_valid(m,second,second_len,1))return PORT_USER_BAD;
    struct span { uintptr_t addr;uint32_t length; } spans[2][3];
    unsigned totals[2]={0,0};
    uint32_t bases[2]={first,second},lengths[2]={first_len,second_len};
    for(unsigned i=0;i<2;i++) {
        uint32_t at=bases[i],left=lengths[i];
        while(left) {
            if(totals[i]>=3u)return PORT_USER_BAD;
            const uint32_t dir=m->pde[at>>22];
            const uint32_t *pte=m->resolve(m->context,dir&0xfffff000u);
            if(!pte)return PORT_USER_BAD;
            const uint32_t entry=pte[(at>>12)&1023u];
            uint8_t *page=physical(context,entry&0xfffff000u);
            if(!page)return PORT_USER_BAD;
            uint32_t offset=at&4095u;
            uint32_t n=4096u-offset;
            if(n>left)n=left;
            spans[i][totals[i]++]=(struct span){(uintptr_t)(page+offset),n};
            at+=n;
            left-=n;
        }
    }
    for(unsigned i=0;i<totals[0];i++) {
        uintptr_t a=spans[0][i].addr;
        for(unsigned j=0;j<totals[1];j++) {
            uintptr_t b=spans[1][j].addr;
            if(a<=b ? (b-a)<spans[0][i].length :
                      (a-b)<spans[1][j].length)
                return PORT_USER_BAD;
        }
    }
    return PORT_USER_OK;
}
