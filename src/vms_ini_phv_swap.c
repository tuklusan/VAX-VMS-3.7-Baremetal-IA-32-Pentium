#include "vms_ini_phv_swap.h"
#include <stdint.h>

static void put16(uint8_t *p,uint16_t x){
    p[0]=(uint8_t)x;
    p[1]=(uint8_t)(x>>8);
}
static uint16_t get16(const uint8_t *p){
    return (uint16_t)((uint16_t)p[0]|(uint16_t)((uint16_t)p[1]<<8));
}
static uint32_t get32(const uint8_t *p){
    return (uint32_t)p[0]|((uint32_t)p[1]<<8)|
           ((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);
}
/* Original allocation orders are exact: first 12+4*(BALSETCT+1)
 * bytes for PHV; next 12+4+4*MAXWSCNT bytes for the swapper PTE map.
 * Header type/size are stored at the same offsets used by INIT.MAR. */
uint32_t port_ini_phvswap_init(void *pool,size_t capacity,
        const struct port_ini_phvswap_settings *s,
        struct port_ini_phvswap *out){
    if(!pool || !s || !out || ((uintptr_t)pool&3u) ||
       s->balance_set_count==UINT32_MAX ||
       s->max_working_set==0u || s->max_working_set>0xFFFFFFu ||
       s->system_pix>UINT16_MAX)return PORT_INI_PHVSWAP_BADPARAM;
    uint64_t slots=(uint64_t)s->balance_set_count+1u;
    uint64_t phvb=12u+4u*slots;
    uint64_t swapb=16u+4u*s->max_working_set;
    uint64_t sum=phvb+swapb;
    /* The DYN size is a WORD. No silently wrapped allocation. */
    if(phvb>UINT16_MAX || swapb>UINT16_MAX || sum>UINT32_MAX)
        return PORT_INI_PHVSWAP_BADPARAM;
    if(sum>capacity)return PORT_INI_PHVSWAP_NOMEM;
    uint8_t *p=(uint8_t *)pool;
    for(uint32_t i=0u;i<(uint32_t)sum;i++)p[i]=0u;
    put16(p+8u,(uint16_t)phvb);
    put16(p+10u,s->phvec_dyn_type);
    uint8_t *pix=p+12u;
    uint8_t *ref=pix+2u*slots;
    /* INI_PHV: last PHV entry belongs to the system header. All
     * preceding slots are free with -1 reference count. */
    for(uint32_t i=0u;i<s->balance_set_count;i++)
        put16(ref+2u*i,UINT16_MAX);
    put16(pix+2u*s->balance_set_count,(uint16_t)s->system_pix);
    put16(ref+2u*s->balance_set_count,1024u);
    uint8_t *sw=p+(uint32_t)phvb;
    put16(sw+8u,(uint16_t)swapb);
    put16(sw+10u,s->swapmap_dyn_type);
    /* INI_SWAP sets PHD$L_P0BR to the swapper PTE array and inserts
     * MAXWSCNT in bits 0..23 of PHD$L_P0LRASTL. */
    *out=(struct port_ini_phvswap){
        .phv_pix_base=pix,.phv_refcount_base=ref,
        .swapper_p0br=sw+12u,
        .phv_slots=(uint32_t)slots,.swapper_p0lr=s->max_working_set,
        .phv_record_bytes=(uint32_t)phvb,
        .swapmap_record_bytes=(uint32_t)swapb,
        .pool_bytes=(uint32_t)sum};
    return PORT_INI_PHVSWAP_OK;
}
uint16_t port_ini_phvswap_pix(const struct port_ini_phvswap *s,uint32_t i){
    if(!s || !s->phv_pix_base || i>=s->phv_slots)return UINT16_MAX;
    return get16(s->phv_pix_base+2u*i);
}
int16_t port_ini_phvswap_refcount(const struct port_ini_phvswap *s,uint32_t i){
    if(!s || !s->phv_refcount_base || i>=s->phv_slots)return INT16_MIN;
    return (int16_t)get16(s->phv_refcount_base+2u*i);
}
uint32_t port_ini_phvswap_pte(const struct port_ini_phvswap *s,uint32_t i){
    if(!s || !s->swapper_p0br || i>=s->swapper_p0lr)return UINT32_MAX;
    return get32(s->swapper_p0br+4u*i);
}
