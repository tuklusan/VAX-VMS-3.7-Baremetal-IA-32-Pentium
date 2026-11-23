#include "vms_iopost_end.h"
#include "vms_v37_dyn_tags.h"
/* IOCIOPOST.MAR 161-165: PID as end-action address skips PCB lookup.
 * Unlike VAX's sign bit, IA-32 supervisor identity mapping is a bounded
 * range. A process I/O packet MUST wait for real PCB/AST/IOSB services. */
uint32_t port_v37_iopost_validate(const struct port_v37_iopost_queue *q,
    uint32_t kernel_begin,uint32_t kernel_limit,uint32_t expected_action){
    uint32_t address=0;const uint8_t *packet=0;
    const uint32_t front=port_v37_iopost_peek(q,&address,&packet);
    if(front!=PORT_V37_IOPOST_OK)return front;
    if(!kernel_begin||kernel_limit<=kernel_begin ||
       expected_action<kernel_begin||expected_action>=kernel_limit)
        return PORT_V37_IOPOST_BAD;
    if(packet[10u]!=PORT_V37_DYN_IRP ||
       port_v37_read16(packet+8u)<PORT_V37_IRP_SIZE ||
       port_v37_read16(packet+8u)>q->packet_stride ||
       port_v37_read32(packet+PORT_V37_IRP_PID)!=expected_action ||
       port_v37_read32(packet+PORT_V37_IRP_AST)!=0u ||
       port_v37_read32(packet+PORT_V37_IRP_WIND)!=0u ||
       port_v37_read32(packet+PORT_V37_IRP_IOSB)!=0u ||
       port_v37_read16(packet+PORT_V37_IRP_STS)!=0u ||
       packet[11u]!=0u ||
       !port_v37_read32(packet+PORT_V37_IRP_UCB))
        return PORT_V37_IOPOST_DEFER;
    return PORT_V37_IOPOST_OK;
}
