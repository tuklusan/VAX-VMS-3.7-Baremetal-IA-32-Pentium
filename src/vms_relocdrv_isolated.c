#include "vms_relocdrv.h"
/* Source DPT initialization must not overwrite its own instruction stream
 * or its read-only indirection constants during the second commit pass. */
static int overlaps(uintptr_t a,size_t an,uintptr_t b,size_t bn){
    if(!an||!bn||a>UINTPTR_MAX-an||b>UINTPTR_MAX-bn)return 1;
    return a<b+bn&&b<a+an;
}
uint32_t port_relocdrv_isolated(const struct port_relocdrv_context *c,
    const uint8_t *table,size_t len){
    if(!c||!table||!len)return PORT_RELOCDRV_BAD;
    /* Read-only literals have their own guest physical address range.
     * Validate that entire span, not only each individual read. */
    if(c->readonly_size&&(!c->readonly_bytes||!c->readonly_base||
       (uint64_t)c->readonly_base+c->readonly_size>UINT32_MAX))
        return PORT_RELOCDRV_BAD;

    for(unsigned i=0;i<PORT_RELOCDRV_REGION_COUNT;i++){
        const struct port_relocdrv_region *a=&c->region[i];
        if(!a->structure_type)continue;
        if(!a->bytes||!a->bytes_size||!a->pc_base||
           (uint64_t)a->pc_base+a->bytes_size>UINT32_MAX||
           (c->readonly_bytes&&c->readonly_size&&
            overlaps((uintptr_t)a->pc_base,a->bytes_size,
                     (uintptr_t)c->readonly_base,c->readonly_size))||
           overlaps((uintptr_t)table,len,(uintptr_t)a->bytes,a->bytes_size)||
           (c->readonly_bytes&&c->readonly_size&&
            overlaps((uintptr_t)c->readonly_bytes,c->readonly_size,
                     (uintptr_t)a->bytes,a->bytes_size)))return PORT_RELOCDRV_BAD;
        for(unsigned j=i+1u;j<PORT_RELOCDRV_REGION_COUNT;j++){
            const struct port_relocdrv_region *b=&c->region[j];
            if(b->structure_type&&(!b->bytes||
               b->structure_type==a->structure_type||
               !b->bytes_size||!b->pc_base||
               (uint64_t)b->pc_base+b->bytes_size>UINT32_MAX||
               overlaps((uintptr_t)a->pc_base,a->bytes_size,
                        (uintptr_t)b->pc_base,b->bytes_size)||
               overlaps((uintptr_t)a->bytes,a->bytes_size,
                        (uintptr_t)b->bytes,b->bytes_size)))
                return PORT_RELOCDRV_BAD;
        }
    }
    return PORT_RELOCDRV_OK;
}
