#include "vms_dq_bootread.h"
/* DQ FDISPATCH first tests physical mode and legal FDT, then uses
 * IRP$W_FUNC and IRP$L_MEDIA. This hardware-independent decode remains
 * strictly bounded to one LBN 1 home block during bootstrap. */
uint32_t port_dq_bootread_decode_physical(const struct port_dq_bootread_request *q,
                                uint32_t *lbn,uint16_t *function,uint32_t requested_lbn){
    if(!q||!q->fdt||!q->irp||!lbn||!function)
        return PORT_DQ_BOOTREAD_BAD;
    const uint16_t func=port_v37_read16(q->irp+PORT_V37_IRP_FUNC);
    const uint32_t media=port_v37_read32(q->irp+PORT_V37_IRP_MEDIA);
    /* ATA LBA28 is the only native device selected here. The original DQ
     * accepts physical block numbers; the adapter boundary is explicit. */
    if(requested_lbn>UINT32_C(0x0fffffff)||media!=requested_lbn)
        return PORT_DQ_BOOTREAD_MEDIA;
    if(func!=PORT_DQ_READPBLK&&func!=PORT_DQ_WRITEPBLK)
        return PORT_DQ_BOOTREAD_UNBOUND;
    struct port_dq_fdt_decision d={0};
    if(port_dq_fdt_classify(q->fdt,sizeof(*q->fdt),func,&d)!=PORT_DQ_FDT_OK ||
       d.action_class!=(func==PORT_DQ_READPBLK?PORT_DQ_FDT_READ:
                                                   PORT_DQ_FDT_WRITE) ||
       d.buffered || d.driver_bound || d.full_function!=func)
        return PORT_DQ_BOOTREAD_UNBOUND;
    *lbn=media;
    *function=func;
    return PORT_DQ_BOOTREAD_OK;
}

uint32_t port_dq_bootread_decode(const struct port_dq_bootread_request *q,
                                uint32_t *lbn,uint16_t *function){
    return port_dq_bootread_decode_physical(q,lbn,function,PORT_DQ_BOOTREAD_LBN);
}
