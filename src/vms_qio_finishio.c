#include "vms_qio_post.h"
#include "vms_v37_dyn_tags.h"
/* SYSQIOREQ.MAR 743-749 EXE$FINISHIO: increment UCB$L_OPCNT, copy
 * R0/R1 to IRP$L_MEDIA (IOST1/IOST2), append to IOC post queue and
 * request IPL$_IOPOST=4. The unported dispatcher has not delivered an
 * IOSB or AST to any process at this stage. */
uint32_t port_v37_qio_finishio(struct port_v37_qio_request *r,
                               uint32_t status,uint32_t second){
    if(!r||!r->post||!r->irp||!r->ucb||
       r->irp_size<PORT_V37_IRP_SIZE||r->ucb_size<PORT_V37_UCB_SIZE||
       !r->irp_va||!r->ucb_va || (r->irp_va&3u)||(r->ucb_va&3u)||
       r->irp[10u]!=PORT_V37_DYN_IRP||r->ucb[10u]!=PORT_V37_DYN_UCB||
       port_v37_read16(r->irp+8u)>r->irp_size||
       port_v37_read16(r->irp+8u)<PORT_V37_IRP_SIZE||
       port_v37_read16(r->ucb+8u)>r->ucb_size||
       port_v37_read16(r->ucb+8u)<PORT_V37_UCB_SIZE||
       port_v37_read32(r->irp+PORT_V37_IRP_UCB)!=r->ucb_va||
       port_v37_read32(r->ucb+PORT_V37_UCB_OPCNT)==UINT32_MAX||
       !r->softint)return PORT_V37_QIO_BAD;
    const uint32_t valid=port_v37_post_validate(r->post);
    if(valid!=PORT_V37_QIO_OK)return valid;
    if(r->irp_va<r->post->first_irp||
       (r->irp_va-r->post->first_irp)%r->post->packet_stride||
       (r->irp_va-r->post->first_irp)/r->post->packet_stride>=r->post->packet_capacity||
       r->irp!=r->post->bytes+(r->irp_va-r->post->first_irp))
        return PORT_V37_QIO_BAD;
    for(uint32_t i=0,cursor=r->post->first;i<r->post->queued;i++){
        if(cursor==r->irp_va)return PORT_V37_QIO_DUP;
        cursor=port_v37_read32(r->post->bytes+(cursor-r->post->first_irp));
    }
    if(r->post->queued==r->post->packet_capacity)return PORT_V37_QIO_FULL;
    port_v37_write32(r->irp+PORT_V37_IRP_MEDIA,status);
    port_v37_write32(r->irp+PORT_V37_IRP_IOST2,second);
    port_v37_write32(r->ucb+PORT_V37_UCB_OPCNT,
        port_v37_read32(r->ucb+PORT_V37_UCB_OPCNT)+1u);
    /* preflight proves append cannot fail; keep a defensive check */
    if(port_v37_post_append(r->post,r->irp_va)!=PORT_V37_QIO_OK)
        return PORT_V37_QIO_CORRUPT;
    r->softint(r->softint_arg,PORT_V37_IOPOST_IPL);
    return PORT_SYS85_NORMAL; /* EXE$FINISHIOC returns service status */
}
