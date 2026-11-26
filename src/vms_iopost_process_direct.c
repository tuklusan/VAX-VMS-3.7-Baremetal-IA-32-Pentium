#include "vms_iopost_end.h"

enum { IRP_PID=12u,IRP_STS=42u,IRP_M_BUFIO=1u };

static uint32_t read16(const uint8_t *p){
    return (uint32_t)p[0]|((uint32_t)p[1]<<8);
}
static uint32_t read32(const uint8_t *p){
    return (uint32_t)p[0]|((uint32_t)p[1]<<8)|
           ((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);
}
static void write16(uint8_t *p,uint16_t v){
    p[0]=(uint8_t)v;p[1]=(uint8_t)(v>>8);
}
static int resolve_irpe(void *context,uint32_t va,const uint8_t **out,
    size_t *bytes){
    const struct port_v37_iopost_process_ops *ops=context;
    return ops->resolve_irpe(ops->context,va,out,bytes);
}
static void unlock_pages(void *context,uint32_t pte,uint32_t pages){
    const struct port_v37_iopost_process_ops *ops=context;
    ops->unlock_pages(ops->context,pte,pages);
}

/* The high-bit PID/end-action branch is handled before this source path.
 * Resolve the current process slot exactly as IOC$IOPOST does, but reject a
 * stale sequence instead of charging a recycled PCB. */
uint32_t port_v37_iopost_process_direct(uint8_t *irp,size_t irp_bytes,
    uint32_t irp_va,const struct port_v37_iopost_process_ops *ops){
    if(!irp||irp_bytes<PORT_V37_IRP_SIZE||!irp_va||(irp_va&3u)||!ops||
       !ops->process_vector||!ops->resolve_pcb||!ops->unlock_pages||
       !ops->queue_kast||ops->maximum_extensions>32u)
        return PORT_V37_IOPOST_BAD;
    const uint32_t pid=read32(irp+IRP_PID);
    const uint32_t status=read16(irp+IRP_STS);
    if(!pid||(pid&UINT32_C(0x80000000))||(status&IRP_M_BUFIO))
        return PORT_V37_IOPOST_PROCESS_DEFER;
    const uint32_t process_index=pid&UINT16_MAX;
    const uint32_t pcb_va=port_ini_procvec_ptr(ops->process_vector,
                                                process_index);
    uint8_t *pcb=0;size_t pcb_bytes=0u;
    if(!pcb_va||(pcb_va&3u)||
       ops->resolve_pcb(ops->context,pcb_va,&pcb,&pcb_bytes)!=0||
       !pcb||pcb_bytes<PORT_V37_PCB_BYTES||((uintptr_t)pcb&3u)||
       read32(pcb+PORT_V37_PCB_PID)!=pid)
        return PORT_V37_IOPOST_PROCESS_DEFER;

    const uint16_t before=(uint16_t)read16(pcb+PORT_V37_PCB_DIOCNT);
    write16(pcb+PORT_V37_PCB_DIOCNT,(uint16_t)(before+1u));
    const struct port_v37_iopost_unlock_ops unlock_ops={
        .context=(void *)ops,
        .resolve_irpe=ops->resolve_irpe?resolve_irpe:0,
        .unlock_pages=unlock_pages,
        .maximum_extensions=ops->maximum_extensions};
    const uint32_t unlocked=port_v37_iopost_unlock_direct(irp,irp_bytes,
                                                           &unlock_ops);
    if(unlocked!=PORT_V37_IOPOST_OK){
        write16(pcb+PORT_V37_PCB_DIOCNT,before);
        return unlocked==PORT_V37_IOPOST_UNLOCK_DEFER?
            PORT_V37_IOPOST_PROCESS_DEFER:unlocked;
    }
    if(ops->queue_kast(ops->context,pid,pcb_va,irp_va)!=
       PORT_V37_IOPOST_OK)
        return PORT_V37_IOPOST_CORRUPT;
    return PORT_V37_IOPOST_OK;
}
