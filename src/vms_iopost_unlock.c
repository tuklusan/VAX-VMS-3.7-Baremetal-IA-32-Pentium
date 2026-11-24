#include "vms_iopost_end.h"

enum {
    IRP_STS=42u, IRP_SVAPTE=44u, IRP_BOFF=48u, IRP_BCNT=52u,
    IRP_EXTEND=84u,
    IRPE_SVAPTE1=44u, IRPE_BOFF1=48u, IRPE_BCNT1=52u,
    IRPE_SVAPTE2=56u, IRPE_BOFF2=60u, IRPE_BCNT2=64u,
    IRPE_EXTEND=84u, IRPE_BYTES=88u,
    IRP_M_PAGIO=1u<<2, IRP_M_VIRTUAL=1u<<4,
    IRP_M_SWAPIO=1u<<6, IRP_M_EXTEND=1u<<11,
    PAGE_SHIFT=9u, PAGE_BYTES=1u<<PAGE_SHIFT,
    MAX_UNLOCKS=1u+2u*32u
};

struct unlock_span { uint32_t pte,pages; };

static uint32_t read16(const uint8_t *p){
    return (uint32_t)p[0]|((uint32_t)p[1]<<8);
}
static uint32_t read32(const uint8_t *p){
    return (uint32_t)p[0]|((uint32_t)p[1]<<8)|
           ((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);
}

static int add_span(struct unlock_span *spans,size_t *count,
    uint32_t pte,uint32_t offset,uint32_t bytes){
    if(!pte)return 1;
    if((pte&3u)||offset>=PAGE_BYTES)return 0;
    const uint64_t extent=(uint64_t)offset+bytes;
    const uint64_t pages=(extent+PAGE_BYTES-1u)>>PAGE_SHIFT;
    if(pages>UINT32_MAX||*count>=MAX_UNLOCKS)return 0;
    if(pages)spans[(*count)++]=(struct unlock_span){pte,(uint32_t)pages};
    return 1;
}

/* IOCIOPOST.MAR DIRECT I/O completion before DIRPOST. The complete IRPE
 * chain is validated first so a corrupt link cannot cause partial unlock. */
uint32_t port_v37_iopost_unlock_direct(uint8_t *irp,size_t irp_bytes,
    const struct port_v37_iopost_unlock_ops *ops){
    if(!irp||irp_bytes<PORT_V37_IRP_SIZE||!ops||!ops->unlock_pages||
       ops->maximum_extensions>32u)
        return PORT_V37_IOPOST_BAD;
    const uint32_t status=read16(irp+IRP_STS);
    if(status&(IRP_M_PAGIO|IRP_M_SWAPIO|IRP_M_VIRTUAL))
        return PORT_V37_IOPOST_UNLOCK_DEFER;

    struct unlock_span spans[MAX_UNLOCKS];
    size_t count=0u;
    if(!add_span(spans,&count,read32(irp+IRP_SVAPTE),
                 read16(irp+IRP_BOFF),read32(irp+IRP_BCNT)))
        return PORT_V37_IOPOST_BAD;

    if(status&IRP_M_EXTEND){
        if(!ops->resolve_irpe||!ops->maximum_extensions)
            return PORT_V37_IOPOST_UNLOCK_DEFER;
        uint32_t next=read32(irp+IRP_EXTEND);
        if(!next)return PORT_V37_IOPOST_BAD;
        for(uint8_t n=0;n<ops->maximum_extensions;n++){
            const uint8_t *extension=0;
            size_t extension_bytes=0u;
            if(!next||(next&3u)||
               ops->resolve_irpe(ops->context,next,&extension,
                                  &extension_bytes)!=0||
               !extension||extension_bytes<IRPE_BYTES)
                return PORT_V37_IOPOST_BAD;
            const uint32_t extension_status=read16(extension+IRP_STS);
            if(!add_span(spans,&count,read32(extension+IRPE_SVAPTE1),
                         read16(extension+IRPE_BOFF1),
                         read32(extension+IRPE_BCNT1))||
               !add_span(spans,&count,read32(extension+IRPE_SVAPTE2),
                         read16(extension+IRPE_BOFF2),
                         read32(extension+IRPE_BCNT2)))
                return PORT_V37_IOPOST_BAD;
            if(!(extension_status&IRP_M_EXTEND)){
                next=0u;
                break;
            }
            next=read32(extension+IRPE_EXTEND);
            /* IOCIOPOST frees the current IRPE before testing the next
             * pointer; a zero next link therefore terminates the chain. */
            if(!next)break;
        }
        if(next)return PORT_V37_IOPOST_BAD;
    }
    for(size_t i=0u;i<count;i++)
        ops->unlock_pages(ops->context,spans[i].pte,spans[i].pages);
    return PORT_V37_IOPOST_OK;
}
