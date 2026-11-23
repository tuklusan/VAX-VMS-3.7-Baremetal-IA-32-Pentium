#include "vms_qio_post.h"
/* Initialize the concrete IOC$GL_PSFL/PSBL sentinel without accepting
 * a host pointer as a VAX/IA32 guest address. */
uint32_t port_v37_post_init(struct port_v37_iopost_queue *q,
    uint8_t *pool,size_t pool_size,uint32_t base,uint32_t stride,
    uint32_t sentinel){
    if(!q || !pool || !base || !sentinel || (base&3u) || (sentinel&3u) ||
       stride<PORT_V37_IRP_SIZE || stride>UINT16_MAX || (stride&3u) ||
       pool_size<stride || pool_size/stride>UINT32_MAX ||
       (uint64_t)base+pool_size>UINT32_MAX ||
       ((uint64_t)sentinel>=base && (uint64_t)sentinel<base+pool_size))
        return PORT_V37_QIO_BAD;
    const uint32_t count=(uint32_t)(pool_size/stride);
    struct port_v37_iopost_queue next={0};
    next.first=sentinel;next.last=sentinel;next.sentinel=sentinel;
    next.first_irp=base;next.packet_stride=stride;
    next.packet_capacity=count;next.bytes=pool;
    *q=next;
    return PORT_V37_QIO_OK;
}
