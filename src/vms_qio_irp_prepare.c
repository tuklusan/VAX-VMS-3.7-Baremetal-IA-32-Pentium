#include "vms_qio_post.h"
#include "vms_v37_dyn_tags.h"
/* SYSDEF.MDL 2827-2880 IRP fields; the caller owns the packet and must
 * remove it from its free queue before building a request. */
uint32_t port_v37_irp_prepare(uint8_t *irp,size_t bytes,uint32_t va,
                             uint32_t ucb,uint16_t func){
    if(!irp||bytes<PORT_V37_IRP_SIZE||bytes>UINT16_MAX||
       !va||(va&3u)||!ucb||(ucb&3u))return PORT_V37_QIO_BAD;
    /* A prepared packet has no source VAX AST, UIC, CCB or IOSB yet. */
    for(size_t i=0;i<PORT_V37_IRP_SIZE;i++)irp[i]=0u;
    port_v37_write16(irp+8u,(uint16_t)bytes);
    irp[10u]=PORT_V37_DYN_IRP;
    port_v37_write32(irp+PORT_V37_IRP_UCB,ucb);
    port_v37_write16(irp+PORT_V37_IRP_FUNC,func);
    return PORT_V37_QIO_OK;
}
