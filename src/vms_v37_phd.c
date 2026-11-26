#include "vms_v37_phd.h"

_Static_assert(PORT_V37_PHD_WSL==PORT_V37_PHD_BYTES,
               "SYSDEFMP.MDL PHD fixed length");
_Static_assert(PORT_V37_PHD_R4==PORT_V37_PHD_KSP+8u*4u,
               "PHD embedded context slot map");
_Static_assert(PORT_V37_PHD_KSP==PORT_V37_PHD_PCB+4u,
               "SYSDEFMP.MDL PHD hardware PCB base");
_Static_assert(PORT_V37_PHD_PC==PORT_V37_PHD_KSP+18u*4u,
               "PHD saved PC slot");

static void put16(uint8_t *p,uint16_t x){
    p[0]=(uint8_t)x;p[1]=(uint8_t)(x>>8);
}
static void put32(uint8_t *p,uint32_t x){
    p[0]=(uint8_t)x;p[1]=(uint8_t)(x>>8);
    p[2]=(uint8_t)(x>>16);p[3]=(uint8_t)(x>>24);
}

uint32_t port_v37_phd_init_genphd(uint8_t *record,size_t capacity,
    const struct port_v37_phd_seed *s){
    if(!record||!s||capacity<PORT_V37_PHD_BYTES||
       !s->native_ksp||(s->native_ksp&3u)||!s->entry_pc||
       !s->p0_page_table||(s->p0_page_table&4095u)||
       !s->p1_page_table||(s->p1_page_table&4095u)||
       s->p0_limit>0x00ffffffu||s->p1_limit>0x00ffffffu||
       s->priority>31u)return PORT_V37_PHD_BADPARAM;
    uint8_t candidate[PORT_V37_PHD_BYTES]={0};
    /* GENPHD Q_PRIVMSK and AUTHPRIV are both -1. */
    for(unsigned i=0;i<8u;i++){
        candidate[PORT_V37_PHD_PRIVMSK+i]=0xffu;
        candidate[PORT_V37_PHD_AUTHPRIV+i]=0xffu;
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
