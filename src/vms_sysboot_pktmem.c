#include "vms_sysboot_pktmem.h"
#include <stddef.h>
/* Native translation of the reverse-address space carving in SYSBOOT.MAR.
 * The caller must map actual PFNs into each [first,resident_end) range;
 * reserved-but-unmapped virtual pages are kept out of the lists. */
static uint32_t round_to(uint32_t n,uint32_t boundary,uint32_t *out){
    uint64_t x=(uint64_t)n+boundary-1u;
    if(!out || x>UINT32_MAX)return 0u;
    *out=(uint32_t)x & ~(boundary-1u);
    return 1u;
}
static uint32_t carve(uint32_t *top,struct port_boo_packet_range *r,
                      uint32_t stride,uint32_t count,uint32_t virtual_count){
    uint64_t bytes=(uint64_t)stride*count;
    uint64_t reserve=(uint64_t)stride*virtual_count;
    uint32_t physical,virtual_bytes;
    if(!stride || stride>UINT16_MAX || count>virtual_count ||
       bytes>UINT32_MAX || reserve>UINT32_MAX ||
       !round_to((uint32_t)bytes,512u,&physical) ||
       !round_to((uint32_t)reserve,512u,&virtual_bytes) ||
       virtual_bytes>*top)return 0u;
    r->first_address=*top-virtual_bytes;
    r->resident_end=r->first_address+physical;
    r->reserved_end=*top;
    r->packet_bytes=stride;
    r->resident_count=count;
    r->virtual_count=virtual_count;
    *top=r->first_address;
    return 1u;
}
uint32_t port_boo_packet_layout(const struct port_boo_packet_settings *s,
                    struct port_boo_packet_layout *layout,
                    struct port_boo_parameters *booparam){
    if(!s || !layout || !booparam || (s->top_address&511u) ||
       !s->top_address || !s->irp_length || !s->srp_requested_size ||
       !s->lrp_requested_size)return PORT_BOO_PACKET_BADPARAM;
    uint32_t irp, srp, lrp_req, lrp;
    if(!round_to(s->irp_length,16u,&irp) ||
       !round_to(s->srp_requested_size,16u,&srp) ||
       irp>0xf0u || irp>UINT16_MAX || srp>UINT16_MAX)
        return PORT_BOO_PACKET_BADPARAM;
    /* VAX SYSBOOT clamps LRP requested size to IRP rounded size,
     * then adds 64 bytes and *rounds down* to a 16-byte boundary. */
    lrp_req=s->lrp_requested_size>irp?s->lrp_requested_size:irp;
    if(lrp_req>UINT16_MAX-64u)return PORT_BOO_PACKET_BADPARAM;
    lrp=(lrp_req+64u)&~15u;
    if(lrp<irp || lrp>UINT16_MAX)return PORT_BOO_PACKET_BADPARAM;
    uint32_t npaged=s->nonpaged_dynamic_bytes&~511u;
    uint32_t nvirt=s->nonpaged_virtual_bytes&~511u;
    uint32_t paged=s->paged_dynamic_bytes&~511u;
    if(nvirt<npaged)nvirt=npaged;
    struct port_boo_packet_layout tmp={0};
    uint32_t top=s->top_address;
    if(!carve(&top,&tmp.srp,srp,s->srp_count,s->srp_virtual_count) ||
       !carve(&top,&tmp.irp,irp,s->irp_count,s->irp_virtual_count) ||
       !carve(&top,&tmp.lrp,lrp,s->lrp_count,s->lrp_virtual_count) ||
       top<nvirt || top-nvirt<paged)return PORT_BOO_PACKET_NOMEM;
    tmp.nonpaged_start=top-nvirt;
    tmp.nonpaged_end=tmp.nonpaged_start+npaged;
    tmp.nonpaged_virtual_end=top;
    tmp.paged_dynamic_start=tmp.nonpaged_start-paged;
    tmp.lrp_minimum=(lrp-(lrp/5u))&~15u;
    struct port_boo_parameters boo={0};
    boo.nonpaged_dynamic_bytes=npaged;
    boo.irp_split_address=tmp.irp.first_address;
    boo.irp_count=s->irp_count;
    boo.lrp_size=lrp;
    boo.lrp_minimum=tmp.lrp_minimum;
    boo.lrp_split_address=tmp.lrp.first_address;
    boo.lrp_count=s->lrp_count;
    boo.srp_split_address=tmp.srp.first_address;
    boo.srp_count=s->srp_count;
    *layout=tmp;
    *booparam=boo;
    return PORT_BOO_PACKET_OK;
}
