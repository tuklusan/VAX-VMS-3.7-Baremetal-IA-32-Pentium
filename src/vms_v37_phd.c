#include "vms_v37_phd.h"

_Static_assert(PORT_V37_PHD_WSL==PORT_V37_PHD_BYTES,
               "SYSDEFMP.MDL PHD fixed length");
_Static_assert(PORT_V37_PHD_R4==PORT_V37_PHD_KSP+8u*4u,
               "PHD embedded context slot map");
_Static_assert(PORT_V37_PHD_KSP==PORT_V37_PHD_PCB+4u,
               "SYSDEFMP.MDL PHD hardware PCB base");
_Static_assert(PORT_V37_PHD_PC==PORT_V37_PHD_KSP+18u*4u,
               "PHD saved PC slot");
_Static_assert(PORT_V37_PHD_WSLIST==PORT_V37_PHD_PRIVMSK_BYTES,
               "SYSDEFMP.MDL PHD working-set list offset");
_Static_assert(PORT_V37_PHD_CPULIM==PORT_V37_PHD_DIOCNT+8u,
               "SYSDEFMP.MDL PHD CPU limit offset");

static void put16(uint8_t *p,uint16_t x){
    p[0]=(uint8_t)x;p[1]=(uint8_t)(x>>8);
}
static uint16_t get16(const uint8_t *p){
    return (uint16_t)p[0]|(uint16_t)((uint16_t)p[1]<<8);
}
static void put32(uint8_t *p,uint32_t x){
    p[0]=(uint8_t)x;p[1]=(uint8_t)(x>>8);
    p[2]=(uint8_t)(x>>16);p[3]=(uint8_t)(x>>24);
}
static void put64(uint8_t *p,uint64_t x){
    for(unsigned i=0u;i<8u;i++)p[i]=(uint8_t)(x>>(8u*i));
}

uint32_t port_v37_phd_init_genphd(uint8_t *record,size_t capacity,
    const struct port_v37_phd_seed *s){
    if(!record||!s||capacity<PORT_V37_PHD_BYTES||
       !s->native_ksp||(s->native_ksp&3u)||!s->entry_pc||
       !s->p0_page_table||(s->p0_page_table&4095u)||
       !s->p1_page_table||(s->p1_page_table&4095u)||
       s->p0_limit>0x00ffffffu||s->p1_limit>0x00ffffffu||
       s->priority>31u||s->ast_level>PORT_V37_PHD_ASTLVL_DISABLED)
        return PORT_V37_PHD_BADPARAM;
    uint8_t candidate[PORT_V37_PHD_BYTES]={0};
    /* PDAT.MAR GENPHD initializes Q_PRIVMSK only. AUTHPRIV remains zero
     * from the source BLKB; granting it here would widen process authority. */
    for(unsigned i=0;i<8u;i++){
        candidate[PORT_V37_PHD_PRIVMSK+i]=0xffu;
    }
    put16(candidate+PORT_V37_PHD_PHVINDEX,UINT16_MAX);
    put32(candidate+PORT_V37_PHD_KSP,s->native_ksp);
    put32(candidate+PORT_V37_PHD_R4,s->initial_r4);
    put32(candidate+PORT_V37_PHD_PC,s->entry_pc);
    put32(candidate+PORT_V37_PHD_P0BR,s->p0_page_table);
    put32(candidate+PORT_V37_PHD_P0LRASTL,
          (s->p0_limit&0x00ffffffu)|((uint32_t)s->ast_level<<24));
    put32(candidate+PORT_V37_PHD_P1BR,s->p1_page_table);
    put32(candidate+PORT_V37_PHD_P1LR,s->p1_limit);
    put32(candidate+PORT_V37_PHD_FREP1VA,s->first_free_p1_va);
    candidate[PORT_V37_PHD_AUTHPRI]=(uint8_t)(31u-s->priority);
    for(size_t i=0;i<sizeof candidate;i++)record[i]=candidate[i];
    return PORT_V37_PHD_OK;
}

uint32_t port_v37_phd_apply_ws_quotas(uint8_t *record,size_t capacity,
    const struct port_v37_phd_ws_quotas *q){
    if(!record||!q||capacity<PORT_V37_PHD_BYTES||
       q->physical_pages<q->free_page_limit||
       q->max_working_set>UINT16_MAX||
       get16(record+PORT_V37_PHD_WSLIST)==0u)
        return PORT_V37_PHD_QUOTA_BADPARAM;

    const uint32_t available=q->physical_pages-q->free_page_limit;
    uint32_t extent=q->extent&UINT16_MAX;
    uint32_t quota=q->quota&UINT16_MAX;
    uint32_t default_size=q->default_size&UINT16_MAX;
    if(extent<quota)extent=quota;
    if((int32_t)extent>(int32_t)available)extent=available;
    if(extent>q->max_working_set)extent=q->max_working_set;
    if(quota>extent)quota=extent;
    if(default_size>quota)default_size=quota;

    const uint16_t bias=(uint16_t)(get16(record+PORT_V37_PHD_WSLIST)-1u);
    const uint16_t phd_extent=(uint16_t)(extent+bias);
    const uint16_t phd_quota=(uint16_t)(quota+bias);
    const uint16_t phd_default=(uint16_t)(default_size+bias);
    put16(record+PORT_V37_PHD_WSEXTENT,phd_extent);
    put16(record+PORT_V37_PHD_WSAUTHEXT,phd_extent);
    put16(record+PORT_V37_PHD_WSQUOTA,phd_quota);
    put16(record+PORT_V37_PHD_WSAUTH,phd_quota);
    put16(record+PORT_V37_PHD_DFWSCNT,phd_default);
    return PORT_V37_PHD_QUOTA_OK;
}

uint32_t port_v37_phd_apply_process_quotas(uint8_t *record,size_t capacity,
    const struct port_v37_phd_process_quotas *q){
    if(!record||!q||capacity<PORT_V37_PHD_BYTES||q->base_priority>31u)
        return PORT_V37_PHD_QUOTA_BADPARAM;
    put32(record+PORT_V37_PHD_CPULIM,q->cpu_limit);
    put16(record+PORT_V37_PHD_ASTLM,(uint16_t)q->ast_limit);
    put64(record+PORT_V37_PHD_PRIVMSK,q->privileges);
    put64(record+PORT_V37_PHD_AUTHPRIV,q->privileges);
    record[PORT_V37_PHD_AUTHPRI]=q->base_priority;
    return PORT_V37_PHD_QUOTA_OK;
}
