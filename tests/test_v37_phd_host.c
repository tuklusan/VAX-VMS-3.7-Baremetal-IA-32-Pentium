#include "vms_v37_phd.h"
#include <stdio.h>
#include <string.h>

static uint32_t get32(const uint8_t *p){
    return (uint32_t)p[0]|((uint32_t)p[1]<<8)|
        ((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);
}
static uint16_t get16(const uint8_t *p){
    return (uint16_t)p[0]|(uint16_t)((uint16_t)p[1]<<8);
}
static int fail(const char *s){fprintf(stderr,"FAIL: %s\n",s);return 1;}

int main(void){
    uint8_t phd[PORT_V37_PHD_BYTES];memset(phd,0xa5,sizeof phd);
    const struct port_v37_phd_seed seed={
        .native_ksp=0x00123000u,.entry_pc=0x00104560u,
        .initial_r4=0x00118000u,.p0_page_table=0x00200000u,
        .p0_limit=0x000fffffu,.p1_page_table=0x00201000u,
        .p1_limit=0x001fffffu,.first_free_p1_va=0x7ffffe00u,
        .priority=16u,.ast_level=4u};
    if(port_v37_phd_init_genphd(phd,sizeof phd,&seed)!=PORT_V37_PHD_OK)
        return fail("valid native PHD seed rejected");
    for(unsigned i=0;i<8u;i++)
        if(phd[PORT_V37_PHD_PRIVMSK+i]!=0xffu||
           phd[PORT_V37_PHD_AUTHPRIV+i]!=0xffu)
            return fail("GENPHD privilege masks");
    if(get16(phd+PORT_V37_PHD_PHVINDEX)!=UINT16_MAX||
       get32(phd+PORT_V37_PHD_KSP)!=seed.native_ksp||
       get32(phd+PORT_V37_PHD_R4)!=seed.initial_r4||
       get32(phd+PORT_V37_PHD_PC)!=seed.entry_pc||
       get32(phd+PORT_V37_PHD_P0BR)!=seed.p0_page_table||
       get32(phd+PORT_V37_PHD_P0LRASTL)!=0x040fffffu||
       get32(phd+PORT_V37_PHD_P1BR)!=seed.p1_page_table||
       get32(phd+PORT_V37_PHD_P1LR)!=seed.p1_limit||
       get32(phd+PORT_V37_PHD_FREP1VA)!=seed.first_free_p1_va||
       phd[PORT_V37_PHD_AUTHPRI]!=15u)
        return fail("GENPHD source fields or native page-table values");
    for(unsigned i=PORT_V37_PHD_AUTHPRI+1u;i<PORT_V37_PHD_BYTES;i++)
        if(phd[i]!=0u)return fail("GENPHD untouched fixed-area bytes not zero");
    uint8_t before[sizeof phd];memcpy(before,phd,sizeof phd);
    struct port_v37_phd_seed bad=seed;bad.p1_page_table|=1u;
    if(port_v37_phd_init_genphd(phd,sizeof phd,&bad)!=PORT_V37_PHD_BADPARAM||
       memcmp(before,phd,sizeof phd))
        return fail("invalid seed was not rejected transactionally");
    if(port_v37_phd_init_genphd(phd,sizeof phd-1u,&seed)!=PORT_V37_PHD_BADPARAM||
       memcmp(before,phd,sizeof phd))
        return fail("short destination was not rejected transactionally");
    puts("SYSDEFMP.MDL PHD layout and PDAT.MAR GENPHD initialization PASS");
    return 0;
}
