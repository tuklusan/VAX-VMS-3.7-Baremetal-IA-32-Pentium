#include "vms_iopost_end.h"

enum { IRP_RMOD=11u,IRP_PID=12u,IRP_AST=16u,IRP_ASTPRM=20u,
       IRP_IOSB=36u,IRP_CHAN=40u,IRP_STS=42u,IRP_EFN=34u,
       IRP_IOST1=56u,IRP_IOST2=60u,IRP_RMOD_QUOTA=0x40u,
       IRP_STS_BUFIO=1u };

/* Source DIRPOST 972-1015. The process AST context supplies PHD accounting;
 * native callbacks preserve the source guarded IOSB, POSTEF, QAST and free
 * operations without pretending they are wired to the scheduler yet. */
uint32_t port_v37_iopost_dirpost(uint8_t *irp,size_t irp_bytes,
    uint32_t irp_va,const struct port_v37_iopost_dirpost_ops *ops){
    if(!irp||irp_bytes<PORT_V37_IRP_SIZE||!irp_va||(irp_va&3u)||!ops||
       !ops->account_io||!ops->ccb_base||!ops->post_event)
        return PORT_V37_IOPOST_BAD;
    const uint32_t pid=port_v37_read32(irp+IRP_PID);
    const uint32_t iosb=port_v37_read32(irp+IRP_IOSB);
    const uint32_t ast=port_v37_read32(irp+IRP_AST);
    const uint8_t request_mode=(uint8_t)(irp[IRP_RMOD]&3u);
    const uint8_t buffered=(uint8_t)((port_v37_read16(irp+IRP_STS)&
                                      IRP_STS_BUFIO)!=0u);
    const uint8_t quota_ast=(uint8_t)((irp[IRP_RMOD]&IRP_RMOD_QUOTA)!=0u);
    if(!pid||(iosb&&!ops->store_iosb_if_writable)||
       (quota_ast&&!ast)||
       (quota_ast&&!ops->queue_ast)||(!quota_ast&&!ops->free_irp))
        return PORT_V37_IOPOST_DIRPOST_DEFER;
    const uint16_t channel=port_v37_read16(irp+IRP_CHAN);
    if(channel<UINT16_C(0x8000))return PORT_V37_IOPOST_BAD;
    const uint32_t offset=UINT32_C(0x10000)-(uint32_t)channel;
    if(!offset||offset>ops->ccb_preceding_bytes||
       offset%PORT_V37_CCB_BYTES||((uintptr_t)ops->ccb_base&3u))
        return PORT_V37_IOPOST_BAD;
    const uint8_t *const ccb_record=ops->ccb_base-offset;
    if(port_v37_read16(ccb_record+PORT_V37_CCB_IOC)==1u&&
       port_v37_read32(ccb_record+PORT_V37_CCB_DIRP)!=0u&&
       (!ops->ccb.wake_acp||
        !port_v37_read32(ccb_record+PORT_V37_CCB_UCB)))
        return PORT_V37_IOPOST_DIRPOST_DEFER;
    ops->account_io(ops->context,pid,buffered);
    uint16_t outstanding=0u;uint32_t deaccess=0u;
    const uint32_t ccb_status=port_v37_iopost_ccb_complete(ops->ccb_base,
        ops->ccb_preceding_bytes,channel,
        &ops->ccb,&outstanding,&deaccess);
    (void)outstanding;(void)deaccess;
    if(ccb_status==PORT_V37_IOPOST_CCB_DEFER)
        return PORT_V37_IOPOST_DIRPOST_DEFER;
    if(ccb_status!=PORT_V37_IOPOST_OK)return PORT_V37_IOPOST_BAD;
    if(iosb)ops->store_iosb_if_writable(ops->context,iosb,request_mode,
        port_v37_read32(irp+IRP_IOST1),port_v37_read32(irp+IRP_IOST2));
    ops->post_event(ops->context,pid,irp[IRP_EFN]);
    if(quota_ast)ops->queue_ast(ops->context,pid,irp_va);
    else ops->free_irp(ops->context,pid,irp_va);
    return PORT_V37_IOPOST_OK;
}
