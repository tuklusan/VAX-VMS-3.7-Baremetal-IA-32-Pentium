#include "vms_exe_memory85.h"

/* SVAPTE.MAR: VMS address region ownership with native IA-32 indices. */
uint32_t port_mmg_pteindex(const struct port_m85_pte_ops *ops,uint32_t va,
                            struct port_m85_pte *out){
    if(!ops||!out||!ops->map||!ops->map->pde)return PORT_M85_ACCVIO;
    if(va<0x40000000u){
        if(va>=ops->regions.first_free_p0)return PORT_M85_LENVIO;
    }else if(va<0x80000000u){
        if(va<=ops->regions.first_free_p1)return PORT_M85_LENVIO;
    }else if(va>=ops->regions.first_free_system)return PORT_M85_LENVIO;
    out->va=va;
    out->vpn=va>>12;
    out->pde_index=(va>>22)&1023u;
    out->pte_index=(va>>12)&1023u;
    out->pte=0;
    out->value=0;
    return PORT_M85_NORMAL;
}
uint32_t port_mmg_pteref(const struct port_m85_pte_ops *ops,uint32_t va,
                          struct port_m85_pte *out){
    uint32_t status=port_mmg_pteindex(ops,va,out);
    if(!(status&1u))return status;
    const uint32_t pde=ops->map->pde[out->pde_index];
    uint32_t *page=0;
    if(pde&1u){
        if(pde&0x80u)return PORT_M85_LENVIO; /* large-page PDE has no PTE */
        if(ops->map->resolve)
            page=(uint32_t *)(uintptr_t)ops->map->resolve(ops->map->context,
                pde&0xFFFFF000u);
    }
    if(!page && ops->page_table_fault)page=ops->page_table_fault(ops->context,va);
    if(!page)return PORT_M85_LENVIO;
    out->pte=&page[out->pte_index];
    out->value=*out->pte;
    return PORT_M85_NORMAL;
}
static uint32_t must_pte(const struct port_m85_pte_ops *ops,uint32_t va,
                          struct port_m85_pte *out){
    uint32_t status=port_mmg_pteref(ops,va,out);
    if(!(status&1u) && ops && ops->fatal_length_violation)
        ops->fatal_length_violation(ops->context,status);
    return status;
}
uint32_t port_mmg_svaptechk(const struct port_m85_pte_ops *ops,uint32_t va,
                             struct port_m85_pte *out){return must_pte(ops,va,out);}
uint32_t port_mmg_pteadrchk(const struct port_m85_pte_ops *ops,uint32_t va,
                             struct port_m85_pte *out){return must_pte(ops,va,out);}

/* BUFFERCTL.MAR: a one-page transfer window with checked physical backing. */
uint32_t port_ioc_filspt(struct port_m85_buffer *b){
    if(!b || !b->map.pde || !b->map.resolve || !b->physical)
        return PORT_M85_ACCVIO;
    const uint32_t va=b->address;
    const uint32_t pde=b->map.pde[(va>>22)&1023u];
    if(!(pde&1u)||!(pde&4u)||(pde&0x80u)||(b->writable && !(pde&2u)))
        return PORT_M85_ACCVIO;
    const uint32_t *table=b->map.resolve(b->map.context,pde&0xFFFFF000u);
    if(!table)return PORT_M85_ACCVIO;
    const uint32_t index=(va>>12)&1023u;
    const uint32_t pte=table[index];
    if(!(pte&1u)||!(pte&4u)||(b->writable && !(pte&2u)))
        return PORT_M85_ACCVIO;
    uint8_t *window=b->physical(b->physical_context,pte&0xFFFFF000u);
    if(!window)return PORT_M85_ACCVIO;
    b->current_pte_index=index;
    b->current_page_phys=pte&0xFFFFF000u;
    b->window=window;
    return PORT_M85_NORMAL;
}
uint32_t port_ioc_initbufwind(struct port_m85_buffer *b,uint32_t va,int write){
    if(!b)return PORT_M85_ACCVIO;
    b->address=va;
    b->writable=(uint8_t)!!write;
    b->window=0;
    return port_ioc_filspt(b);
}
uint32_t port_ioc_getbyte(struct port_m85_buffer *b,uint8_t *byte){
    if(!b||!byte||b->writable)return PORT_M85_ACCVIO;
    if(!b->window && port_ioc_filspt(b)!=PORT_M85_NORMAL)return PORT_M85_ACCVIO;
    *byte=b->window[b->address&4095u];
    b->address++;
    if(!(b->address&4095u))b->window=0;
    return PORT_M85_NORMAL;
}
uint32_t port_ioc_putbyte(struct port_m85_buffer *b,uint8_t byte){
    if(!b||!b->writable)return PORT_M85_ACCVIO;
    if(!b->window && port_ioc_filspt(b)!=PORT_M85_NORMAL)return PORT_M85_ACCVIO;
    b->window[b->address&4095u]=byte;
    b->address++;
    if(!(b->address&4095u))b->window=0;
    return PORT_M85_NORMAL;
}
uint32_t port_ioc_movfruser(struct port_m85_buffer *b,uint8_t *dst,uint32_t n){
    if(!b||(!dst&&n)||b->writable)return PORT_M85_ACCVIO;
    if(!b->physical)return PORT_M85_ACCVIO;
    while(n){
        uint32_t take=n>PORT_USER_RANGE_MAX?PORT_USER_RANGE_MAX:n;
        if(port_user_copy_from(&b->map,dst,b->address,take,
            b->physical,b->physical_context)!=PORT_USER_OK)return PORT_M85_ACCVIO;
        dst+=take;
        b->address+=take;
        n-=take;
    }
    b->window=0;
    return PORT_M85_NORMAL;
}
uint32_t port_ioc_movtouser(struct port_m85_buffer *b,const uint8_t *src,uint32_t n){
    if(!b||(!src&&n)||!b->writable)return PORT_M85_ACCVIO;
    if(!b->physical)return PORT_M85_ACCVIO;
    while(n){
        uint32_t take=n>PORT_USER_RANGE_MAX?PORT_USER_RANGE_MAX:n;
        if(port_user_copy_to(&b->map,b->address,src,take,
            b->physical,b->physical_context)!=PORT_USER_OK)return PORT_M85_ACCVIO;
        src+=take;
        b->address+=take;
        n-=take;
    }
    b->window=0;
    return PORT_M85_NORMAL;
}
