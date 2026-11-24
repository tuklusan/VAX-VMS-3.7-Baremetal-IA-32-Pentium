#include "vms_exe_np_pool.h"
#include "vms_qio_post.h"
#include "vms_v37_dyn_tags.h"
#include <stdint.h>

_Static_assert(sizeof(struct port_exe_np_free)==8u,
               "IA-32 MEMORYALC free block prefix");
static uint32_t lock_pool(const struct port_exe_np_pool *p){
    return p->sync->raise_sync(p->sync->context);
}
static void unlock_pool(const struct port_exe_np_pool *p,uint32_t old){
    p->sync->restore_ipl(p->sync->context,old);
}
static struct port_exe_np_free *node_at(const struct port_exe_np_pool *p,
                                        uint32_t offset){
    return offset==UINT32_MAX?0:(struct port_exe_np_free *)(p->base+offset);
}
static uint32_t node_offset(const struct port_exe_np_pool *p,
                            const struct port_exe_np_free *node){
    return node?(uint32_t)((const uint8_t *)node-p->base):UINT32_MAX;
}
uint32_t port_exe_np_pool_init(struct port_exe_np_pool *p,void *memory,
    size_t bytes,const struct port_v37_irp_pool_sync *sync){
    if(!p||!memory||!sync||!sync->raise_sync||!sync->restore_ipl||
       ((uintptr_t)memory&7u)||bytes<sizeof(struct port_exe_np_free)||
       bytes>UINT32_MAX||(bytes&7u))return PORT_EXE_NP_POOL_BAD;
    struct port_exe_np_free *first=memory;
    first->next_offset=UINT32_MAX;first->bytes=(uint32_t)bytes;
    *p=(struct port_exe_np_pool){.base=memory,.bytes=(uint32_t)bytes,
        .head_offset=0u,.allocations=0u,.sync=sync};
    return PORT_EXE_NP_POOL_OK;
}
uint32_t port_exe_np_pool_check(const struct port_exe_np_pool *p){
    if(!p||!p->base||!p->bytes||!p->sync)return PORT_EXE_NP_POOL_BAD;
    uintptr_t low=(uintptr_t)p->base,high=low+p->bytes;
    if(high<low)return PORT_EXE_NP_POOL_CORRUPT;
    uintptr_t previous=0u;uint32_t traversed=0u;
    const struct port_exe_np_free *b=node_at(p,p->head_offset);
    while(b){
        uintptr_t at=(uintptr_t)b;
        if(at<low||at>high-sizeof(*b)||(at&7u)||
           b->bytes<sizeof(*b)||(b->bytes&7u)||
           (uint64_t)at+b->bytes>high||at<=previous||
           (b->next_offset!=UINT32_MAX&&
            ((b->next_offset&7u)||b->next_offset>p->bytes-sizeof(*b)||
            (uint64_t)(at-low)+b->bytes>=b->next_offset))||
           ++traversed>p->bytes/sizeof(*b))
            return PORT_EXE_NP_POOL_CORRUPT;
        previous=at;b=node_at(p,b->next_offset);
    }
    return PORT_EXE_NP_POOL_OK;
}
uint32_t port_exe_np_pool_alloc(struct port_exe_np_pool *p,uint32_t requested,
    void **address,uint32_t *allocated_bytes){
    if(!p||!address||!allocated_bytes||!requested||requested>UINT32_MAX-7u)
        return PORT_EXE_NP_POOL_BAD;
    uint32_t size=(requested+7u)&~7u;
    uint32_t old=lock_pool(p);
    struct port_exe_np_free *previous=0,*block=node_at(p,p->head_offset);
    while(block&&block->bytes<size){previous=block;block=node_at(p,block->next_offset);}
    if(!block){unlock_pool(p,old);return PORT_EXE_NP_POOL_EMPTY;}
    if(p->allocations==UINT32_MAX){
        unlock_pool(p,old);return PORT_EXE_NP_POOL_CORRUPT;
    }
    struct port_exe_np_free *next=node_at(p,block->next_offset);
    uint32_t remaining=block->bytes-size;
    if(remaining){
        struct port_exe_np_free *tail=(void *)((uint8_t *)block+size);
        tail->next_offset=node_offset(p,next);tail->bytes=remaining;
        if(previous)previous->next_offset=node_offset(p,tail);
        else p->head_offset=node_offset(p,tail);
    }else if(previous)previous->next_offset=node_offset(p,next);
    else p->head_offset=node_offset(p,next);
    ++p->allocations;*address=block;*allocated_bytes=size;
    unlock_pool(p,old);return PORT_EXE_NP_POOL_OK;
}
uint32_t port_exe_np_pool_free(struct port_exe_np_pool *p,void *address,
    uint32_t requested){
    if(!p||!address||!requested||requested>UINT32_MAX-7u)
        return PORT_EXE_NP_POOL_BAD;
    uint32_t size=(requested+7u)&~7u;
    uintptr_t low=(uintptr_t)p->base,high=low+p->bytes,at=(uintptr_t)address;
    if(at<low||at>=high||(at&7u)||(uint64_t)at+size>high)
        return PORT_EXE_NP_POOL_BAD;
    uint32_t old=lock_pool(p);
    struct port_exe_np_free *previous=0,*next=node_at(p,p->head_offset);
    while(next&&(uintptr_t)next<at){previous=next;next=node_at(p,next->next_offset);}
    if((previous&&(uint64_t)(uintptr_t)previous+previous->bytes>at)||
       (next&&(uint64_t)at+size>(uintptr_t)next)||!p->allocations){
        unlock_pool(p,old);return PORT_EXE_NP_POOL_DUP;
    }
    struct port_exe_np_free *block=address;
    block->next_offset=node_offset(p,next);block->bytes=size;
    if(previous)previous->next_offset=node_offset(p,block);
    else p->head_offset=node_offset(p,block);
    if(next&&(uint64_t)at+block->bytes==(uintptr_t)next){
        block->bytes+=next->bytes;block->next_offset=next->next_offset;
    }
    if(previous&&(uint64_t)(uintptr_t)previous+previous->bytes==(uintptr_t)block){
        previous->bytes+=block->bytes;previous->next_offset=block->next_offset;
    }
    --p->allocations;
    unlock_pool(p,old);return PORT_EXE_NP_POOL_OK;
}

static uint32_t round8(uint32_t n){return (n+7u)&~7u;}
static uint32_t round16(uint32_t n){return (n+15u)&~15u;}
static struct port_ini_packet_list *lookaside_at(
    const struct port_exe_np_allocator *a,uintptr_t address,uint8_t *source,
    uint32_t *guest_address){
    struct port_ini_packet_list *const lists[3]={a->irps,a->lrps,a->srps};
    for(unsigned i=0;i<3u;i++){
        struct port_ini_packet_list *q=lists[i];
        if(!q||!q->memory||!q->packet_count||!q->packet_bytes)continue;
        uint64_t used=(uint64_t)q->packet_count*q->packet_bytes;
        uintptr_t host_start=(uintptr_t)q->memory,host_end=host_start+used;
        uint32_t offset=UINT32_MAX;
        if(address>=host_start&&address<host_end)
            offset=(uint32_t)(address-host_start);
        else if(address<=UINT32_MAX&&address>=q->split_address&&
                (uint64_t)address<(uint64_t)q->split_address+used)
            offset=(uint32_t)address-q->split_address;
        if(offset!=UINT32_MAX&&offset%q->packet_bytes==0u){
            if(source)*source=(uint8_t)(i+1u);
            if(guest_address)*guest_address=q->split_address+offset;
            return q;
        }
    }
    return 0;
}
uint32_t port_exe_np_allocate(struct port_exe_np_allocator *a,
    uint32_t requested,uint8_t tag,struct port_exe_np_allocation *out){
    if(!a||!a->variable||!a->sync||!out||!requested||!tag||
       requested>UINT32_MAX-15u)return PORT_EXE_NP_POOL_BAD;
    const uint32_t size=round8(requested);
    struct port_ini_packet_list *q=0;uint8_t source=PORT_EXE_NP_FROM_VARIABLE;
    if(size>round8(PORT_V37_IRP_SIZE)){
        q=a->lrps;source=PORT_EXE_NP_FROM_LRP;
    }else if(a->irps&&size<a->irps->minimum_bytes){
        q=a->srps;source=PORT_EXE_NP_FROM_SRP;
    }else{
        q=a->irps;source=PORT_EXE_NP_FROM_IRP;
    }
    if(q){
        const uint32_t rounded_packet=round16(size);
        if((source!=PORT_EXE_NP_FROM_SRP&&size<q->minimum_bytes)||
           size>q->packet_bytes||
           rounded_packet>q->packet_bytes)q=0;
    }
    if(q){
        uint8_t *packet=0;uint32_t address=0;
        const uint32_t st=port_v37_lookaside_alloc(q,size,tag,a->sync,
                                                   &packet,&address);
        if(st==PORT_V37_IRP_POOL_OK){
            *out=(struct port_exe_np_allocation){packet,q->packet_bytes,
                                                  source,tag};
            return PORT_EXE_NP_POOL_OK;
        }
        if(st!=PORT_V37_IRP_POOL_EMPTY)
            return st==PORT_V37_IRP_POOL_CORRUPT?
                PORT_EXE_NP_POOL_CORRUPT:PORT_EXE_NP_POOL_BAD;
    }
    void *memory=0;uint32_t allocated=0;
    const uint32_t st=port_exe_np_pool_alloc(a->variable,requested,
                                              &memory,&allocated);
    if(st!=PORT_EXE_NP_POOL_OK)return st;
    *out=(struct port_exe_np_allocation){memory,allocated,
                                         PORT_EXE_NP_FROM_VARIABLE,tag};
    return PORT_EXE_NP_POOL_OK;
}
uint32_t port_exe_np_release(struct port_exe_np_allocator *a,
    const struct port_exe_np_allocation *allocation){
    if(!a||!allocation||!allocation->address||!allocation->bytes||
       !allocation->tag||!a->variable||!a->sync)
        return PORT_EXE_NP_POOL_BAD;
    uint32_t va=(uint32_t)(uintptr_t)allocation->address;
    uint8_t source=PORT_EXE_NP_FROM_VARIABLE;
    struct port_ini_packet_list *q=lookaside_at(a,(uintptr_t)allocation->address,
                                                &source,&va);
    if(q){
        if(source!=allocation->source&&
           allocation->source!=PORT_EXE_NP_FROM_VARIABLE)
            return PORT_EXE_NP_POOL_BAD;
        uint8_t *packet=q->memory+(va-q->split_address);
        return port_v37_lookaside_free(q,packet,va,q->packet_bytes,
                                       allocation->tag,
                                       a->sync)==PORT_V37_IRP_POOL_OK?
            PORT_EXE_NP_POOL_OK:PORT_EXE_NP_POOL_CORRUPT;
    }
    if(allocation->source!=PORT_EXE_NP_FROM_VARIABLE)
        return PORT_EXE_NP_POOL_BAD;
    return port_exe_np_pool_free(a->variable,allocation->address,
                                 allocation->bytes);
}
uint32_t port_exe_np_deallocate(struct port_exe_np_allocator *a,void *address,
    uint32_t requested,uint8_t tag){
    if(!a||!address||!requested||!tag||!a->variable)
        return PORT_EXE_NP_POOL_BAD;
    uint8_t source=PORT_EXE_NP_FROM_VARIABLE;
    uint32_t guest_address=0u;
    struct port_ini_packet_list *q=lookaside_at(a,(uintptr_t)address,&source,
                                                &guest_address);
    (void)guest_address;
    struct port_exe_np_allocation allocation={
        address,q?q->packet_bytes:round8(requested),source,tag};
    return port_exe_np_release(a,&allocation);
}
