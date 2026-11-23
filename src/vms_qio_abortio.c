#include "vms_qio_post.h"
#include "vms_v37_dyn_tags.h"

/* SYSQIOREQ.MAR 736-740 EXE$ABORTIO. Clear IRP$L_IOSB, return the AST
 * reservation only when ACB$V_QUOTA was set, enqueue, then request IPL4.
 * PCB lookup and caller-memory/status postprocessing remain unconnected. */
uint32_t port_v37_qio_abortio(struct port_v37_qio_request *r,int16_t *ast_count){
    if(!r||!r->post||!r->irp||!ast_count||!r->irp_va||
       (r->irp_va&3u)||r->irp_size<PORT_V37_IRP_SIZE||!r->softint||
       r->irp[10u]!=PORT_V37_DYN_IRP||
       port_v37_read16(r->irp+8u)<PORT_V37_IRP_SIZE||
       port_v37_read16(r->irp+8u)>r->irp_size)
        return PORT_V37_QIO_BAD;
    struct port_v37_iopost_queue *q=r->post;
    uint32_t valid=port_v37_post_validate(q);
    if(valid!=PORT_V37_QIO_OK)return valid;
    if(r->irp_va<q->first_irp||
       (r->irp_va-q->first_irp)%q->packet_stride||
       (r->irp_va-q->first_irp)/q->packet_stride>=q->packet_capacity||
       r->irp!=q->bytes+(r->irp_va-q->first_irp))return PORT_V37_QIO_BAD;
    for(uint32_t i=0,cursor=q->first;i<q->queued;i++){
        if(cursor==r->irp_va)return PORT_V37_QIO_DUP;
        cursor=port_v37_read32(q->bytes+(cursor-q->first_irp));
    }
    if(q->queued==q->packet_capacity)return PORT_V37_QIO_FULL;
    uint8_t rmod=r->irp[PORT_V37_IRP_RMOD];
    port_v37_write32(r->irp+PORT_V37_QIO_IRP_IOSB,0u);
    if(rmod&0x40u){
        r->irp[PORT_V37_IRP_RMOD]=(uint8_t)(rmod&~0x40u);
        *ast_count=*ast_count==INT16_MAX?INT16_MIN:(int16_t)(*ast_count+1);
    }
    if(port_v37_post_append(q,r->irp_va)!=PORT_V37_QIO_OK)
        return PORT_V37_QIO_CORRUPT;
    r->softint(r->softint_arg,PORT_V37_IOPOST_IPL);
    return PORT_SYS85_NORMAL;
}
