#include "vms_exe_paged_pool.h"

uint32_t port_exe_paged_pool_init(struct port_exe_paged_pool *pool,
    void *memory,size_t bytes,const struct port_v37_irp_pool_sync *sync){
    if(!pool||!memory||((uintptr_t)memory&15u)||(bytes&15u)||bytes<16u)
        return PORT_EXE_PAGED_BAD;
    const uint32_t status=port_exe_np_pool_init(&pool->free_blocks,memory,
                                                bytes,sync);
    return status==PORT_EXE_NP_POOL_OK?PORT_EXE_PAGED_OK:PORT_EXE_PAGED_BAD;
}

uint32_t port_exe_paged_alloc(struct port_exe_paged_pool *pool,
    uint32_t requested,void **address,uint32_t *allocated_bytes){
    if(!pool||!requested||requested>UINT32_MAX-15u||!address||!allocated_bytes)
        return PORT_EXE_PAGED_BAD;
    const uint32_t size=(requested+15u)&~15u;
    void *candidate=0;uint32_t actual=0;
    const uint32_t status=port_exe_np_pool_alloc(&pool->free_blocks,size,
                                                  &candidate,&actual);
    if(status==PORT_EXE_NP_POOL_EMPTY)return PORT_EXE_PAGED_EMPTY;
    if(status!=PORT_EXE_NP_POOL_OK)return PORT_EXE_PAGED_CORRUPT;
    if(actual!=size||((uintptr_t)candidate&15u)){
        (void)port_exe_np_pool_free(&pool->free_blocks,candidate,actual);
        return PORT_EXE_PAGED_CORRUPT;
    }
    *address=candidate;*allocated_bytes=actual;
    return PORT_EXE_PAGED_OK;
}

uint32_t port_exe_paged_free(struct port_exe_paged_pool *pool,void *address,
    uint32_t bytes){
    if(!pool||!address||!bytes||bytes>UINT32_MAX-15u||
       ((uintptr_t)address&15u))
        return PORT_EXE_PAGED_BAD;
    const uint32_t rounded=(bytes+15u)&~15u;
    const uint32_t status=port_exe_np_pool_free(&pool->free_blocks,address,rounded);
    if(status==PORT_EXE_NP_POOL_OK)return PORT_EXE_PAGED_OK;
    if(status==PORT_EXE_NP_POOL_DUP)return PORT_EXE_PAGED_DUP;
    return PORT_EXE_PAGED_BAD;
}
