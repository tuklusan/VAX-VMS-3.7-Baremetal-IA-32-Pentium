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
           phd[PORT_V37_PHD_AUTHPRIV+i]!=0u)
            return fail("GENPHD privilege mask and zero AUTHPRIV");
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
    const struct port_v37_phd_process_quotas process_quotas={
        .cpu_limit=0xa1b2c3d4u,.ast_limit=0x12345u,
        .privileges=UINT64_C(0x0123456789abcdef),.base_priority=23u};
    if(port_v37_phd_apply_process_quotas(phd,sizeof phd,&process_quotas)!=
           PORT_V37_PHD_QUOTA_OK||
       get32(phd+PORT_V37_PHD_CPULIM)!=process_quotas.cpu_limit||
       get16(phd+PORT_V37_PHD_ASTLM)!=0x2345u||
       phd[PORT_V37_PHD_AUTHPRI]!=process_quotas.base_priority)
        return fail("PROCSTRT CPU/AST limit and authorized priority");
    for(unsigned i=0u;i<8u;i++){
        const uint8_t expected=(uint8_t)(process_quotas.privileges>>(8u*i));
        if(phd[PORT_V37_PHD_PRIVMSK+i]!=expected||
           phd[PORT_V37_PHD_AUTHPRIV+i]!=expected)
            return fail("PROCSTRT active and authorized privilege masks");
    }
    phd[PORT_V37_PHD_WSLIST]=64u;phd[PORT_V37_PHD_WSLIST+1u]=0u;
    const struct port_v37_phd_ws_quotas quotas={
        .physical_pages=300u,.free_page_limit=20u,.max_working_set=256u,
        .extent=300u,.quota=150u,.default_size=400u};
    if(port_v37_phd_apply_ws_quotas(phd,sizeof phd,&quotas)!=
           PORT_V37_PHD_QUOTA_OK||
       get16(phd+PORT_V37_PHD_WSEXTENT)!=319u||
       get16(phd+PORT_V37_PHD_WSAUTHEXT)!=319u||
       get16(phd+PORT_V37_PHD_WSQUOTA)!=213u||
       get16(phd+PORT_V37_PHD_WSAUTH)!=213u||
       get16(phd+PORT_V37_PHD_DFWSCNT)!=213u)
        return fail("PROCSTRT working-set extent/quota/default normalization");
    uint8_t before[sizeof phd];memcpy(before,phd,sizeof phd);
    struct port_v37_phd_ws_quotas bad_quotas=quotas;
    bad_quotas.physical_pages=10u;bad_quotas.free_page_limit=20u;
    if(port_v37_phd_apply_ws_quotas(phd,sizeof phd,&bad_quotas)!=
           PORT_V37_PHD_QUOTA_BADPARAM||memcmp(before,phd,sizeof phd)||
       port_v37_phd_apply_ws_quotas(phd,sizeof phd-1u,&quotas)!=
           PORT_V37_PHD_QUOTA_BADPARAM||memcmp(before,phd,sizeof phd))
        return fail("invalid PROCSTRT quota input is transactionally rejected");
    struct port_v37_phd_seed bad=seed;bad.p1_page_table|=1u;
    if(port_v37_phd_init_genphd(phd,sizeof phd,&bad)!=PORT_V37_PHD_BADPARAM||
       memcmp(before,phd,sizeof phd))
        return fail("invalid seed was not rejected transactionally");
    if(port_v37_phd_init_genphd(phd,sizeof phd-1u,&seed)!=PORT_V37_PHD_BADPARAM||
       memcmp(before,phd,sizeof phd))
        return fail("short destination was not rejected transactionally");
    bad=seed;bad.ast_level=PORT_V37_PHD_ASTLVL_DISABLED+1u;
    if(port_v37_phd_init_genphd(phd,sizeof phd,&bad)!=PORT_V37_PHD_BADPARAM||
       memcmp(before,phd,sizeof phd))
        return fail("reserved SYSDEFMP ASTLVL rejected transactionally");
    puts("PHD GENPHD, PROCSTRT CPU/AST/privileges and WS quota setup PASS");
    return 0;
}
