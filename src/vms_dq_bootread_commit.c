#include "vms_dq_bootread.h"
/* Source DQDRIVER PREPROCESS/FUNCXT: commit *after* successful hardware
 * operation, leaving failed/preflight requests and UCB intact. Physical
 * boot READPBLK bypasses volume VALID exactly as source FDISPATCH does. */
uint32_t port_dq_bootread_commit(const struct port_dq_bootread_request *q,
    const uint16_t sector[256],struct port_dq_bootread_result *out){
    if(!q||!sector||!out||!q->irp||!q->sector||!q->device_db||
       q->sector_bytes!=PORT_DQ_BOOTREAD_BYTES)
        return PORT_DQ_BOOTREAD_BAD;
    uint8_t *u=(uint8_t *)(uintptr_t)(q->device_db+PORT_V37_UCB_BOOT_OFF);
    const uint32_t ops=port_v37_read32(u+PORT_V37_UCB_OPCNT);
    if(ops==UINT32_MAX)return PORT_DQ_BOOTREAD_BAD;
    for(unsigned i=0;i<256u;i++)q->sector[i]=sector[i];
    const uint32_t block=port_v37_read32(q->irp+PORT_V37_IRP_MEDIA);
    port_v37_write32(u+PORT_DQ_UCB_MEDIA,block);
    port_v37_write16(u+PORT_DQ_UCB_FUNC,PORT_DQ_READPBLK);
    u[PORT_DQ_UCB_FEX]=(uint8_t)PORT_DQ_READPBLK;
    u[112u]=u[113u]; /* source retry count initialization */
    port_v37_write16(u+PORT_DQ_UCB_BCR,0u);
    port_v37_write32(u+PORT_V37_UCB_OPCNT,ops+1u);
    /* SYSQIOREQ EXE$FINISHIOC source IRP fields: IOST1 status and IOST2
     * transferred bytes; no event flag or user-visible IOSB is posted. */
    port_v37_write32(q->irp+PORT_V37_IRP_MEDIA,PORT_SYS85_NORMAL);
    port_v37_write32(q->irp+PORT_V37_IRP_IOST2,PORT_DQ_BOOTREAD_BYTES);
    *out=(struct port_dq_bootread_result){
        .lbn=block,.bytes=PORT_DQ_BOOTREAD_BYTES,
        .irp_status=port_v37_read32(q->irp+PORT_V37_IRP_MEDIA),
        .irp_second=port_v37_read32(q->irp+PORT_V37_IRP_IOST2),
        .ucb_media=port_v37_read32(u+PORT_DQ_UCB_MEDIA),
        .ucb_io_count=ops+1u,.io_function=PORT_DQ_READPBLK,
        .unit=q->boot_unit,.sector_checksum=port_dq_bootread_checksum(sector),
        .software_online=(uint16_t)((port_v37_read16(u+88u)&PORT_V37_UCB_ONLINE)!=0u)};
    return PORT_DQ_BOOTREAD_OK;
}
