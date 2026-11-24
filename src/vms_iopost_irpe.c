#include "vms_iopost_end.h"

enum { IRP_STS=42u,IRP_EXTEND=84u,IRPE_STS=42u,IRPE_EXTEND=84u,
       IRP_M_EXTEND=1u<<11 };

static uint32_t read16(const uint8_t *p){
    return (uint32_t)p[0]|((uint32_t)p[1]<<8);
}
static uint32_t read32(const uint8_t *p){
    return (uint32_t)p[0]|((uint32_t)p[1]<<8)|
           ((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);
}

/* IOCIOPOST.MAR 1016-1034: validate the root IRPE chain before DIRPOST
 * changes accounting or the CCB, then release each extension after POSTEF. */
uint32_t port_v37_iopost_irpe_plan(const uint8_t *irp,size_t irp_bytes,
    port_v37_iopost_resolve_irpe_fn resolve,void *context,
    uint8_t maximum,struct port_v37_iopost_irpe_plan *out){
    if(!irp||irp_bytes<PORT_V37_IRP_SIZE||!out||
       maximum>PORT_V37_IOPOST_MAX_IRPES)
        return PORT_V37_IOPOST_BAD;
    struct port_v37_iopost_irpe_plan plan={0};
    if(!(read16(irp+IRP_STS)&IRP_M_EXTEND)){*out=plan;return PORT_V37_IOPOST_OK;}
    if(!resolve||!maximum)return PORT_V37_IOPOST_IRPE_DEFER;
    uint32_t next=read32(irp+IRP_EXTEND);
    if(!next||(next&3u))return PORT_V37_IOPOST_BAD;
    for(uint8_t i=0u;i<maximum;i++){
        for(uint8_t j=0u;j<plan.count;j++)
            if(plan.address[j]==next)return PORT_V37_IOPOST_BAD;
        const uint8_t *extension=0;size_t bytes=0u;
        if(resolve(context,next,&extension,&bytes)!=0||!extension||
           bytes<PORT_V37_IRPE_BYTES||((uintptr_t)extension&3u))
            return PORT_V37_IOPOST_BAD;
        plan.address[plan.count++]=next;
        if(!(read16(extension+IRPE_STS)&IRP_M_EXTEND)){
            *out=plan;return PORT_V37_IOPOST_OK;
        }
        next=read32(extension+IRPE_EXTEND);
        /* Source saves this value, frees the current IRPE, then tests the
         * pointer; a zero successor terminates the chain. */
        if(!next){*out=plan;return PORT_V37_IOPOST_OK;}
        if(next&3u)return PORT_V37_IOPOST_BAD;
    }
    return PORT_V37_IOPOST_BAD;
}

uint32_t port_v37_iopost_irpe_release(
    const struct port_v37_iopost_irpe_plan *plan,
    port_v37_iopost_packet_fn free_irpe,void *context,uint32_t pid){
    if(!plan||plan->count>PORT_V37_IOPOST_MAX_IRPES||
       (plan->count&&(!free_irpe||!pid)))return PORT_V37_IOPOST_BAD;
    for(uint8_t i=0u;i<plan->count;i++){
        if(!plan->address[i]||(plan->address[i]&3u))
            return PORT_V37_IOPOST_BAD;
        for(uint8_t j=0u;j<i;j++)
            if(plan->address[j]==plan->address[i])return PORT_V37_IOPOST_BAD;
    }
    for(uint8_t i=0u;i<plan->count;i++)
        free_irpe(context,pid,plan->address[i]);
    return PORT_V37_IOPOST_OK;
}
