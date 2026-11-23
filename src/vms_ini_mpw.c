#include "vms_ini_mpw.h"
#include <stdint.h>
static void write16(uint8_t *p,uint16_t x){p[0]=(uint8_t)x;p[1]=(uint8_t)(x>>8);}
static uint16_t read16(const uint8_t *p){return (uint16_t)p[0]|(uint16_t)((uint16_t)p[1]<<8);}
static uint32_t read32(const uint8_t *p){
    return (uint32_t)p[0]|((uint32_t)p[1]<<8)|
           ((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);
}
uint32_t port_ini_mpw_init(void *pool,size_t capacity,
             const struct port_ini_mpw_settings *s,struct port_ini_mpw *out){
    if(!pool || !s || !out || ((uintptr_t)pool&3u) ||
       s->requested_fault_cluster>UINT16_MAX)
        return PORT_INI_MPW_BADPARAM;
    /* BICL #7,R1 / BNEQ ... MOVL #16,R1 in original INIT.MAR. */
    uint32_t pfc=s->requested_fault_cluster&~7u;
    if(!pfc)pfc=16u;
    uint32_t slots=pfc+7u;
    /* ROTL #2,R1 -> 4*slots; word-indexed MOVAW adds 2*slots. */
    uint32_t bytes=12u+6u*slots;
    if(bytes>UINT16_MAX)return PORT_INI_MPW_BADPARAM;
    if((size_t)bytes>capacity)return PORT_INI_MPW_NOMEM;
    uint8_t *p=(uint8_t *)pool;
    for(uint32_t i=0;i<bytes;i++)p[i]=0u;
    write16(p+8u,(uint16_t)bytes);
    write16(p+10u,s->dyn_mpwmap_type);
    *out=(struct port_ini_mpw){
        .pte_base=p+12u,.phv_index_base=p+12u+slots*4u,
        .effective_fault_cluster=pfc,.swap_increment=pfc,
        .allocated_slots=slots,.allocated_bytes=bytes};
    return PORT_INI_MPW_OK;
}
uint32_t port_ini_mpw_pte(const struct port_ini_mpw *r,uint32_t i){
    return (!r || !r->pte_base || i>=r->allocated_slots)?UINT32_MAX:
           read32(r->pte_base+i*4u);
}
uint16_t port_ini_mpw_phv_index(const struct port_ini_mpw *r,uint32_t i){
    return (!r || !r->phv_index_base || i>=r->allocated_slots)?UINT16_MAX:
           read16(r->phv_index_base+i*2u);
}
