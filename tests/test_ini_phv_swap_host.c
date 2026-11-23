#include "vms_ini_phv_swap.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#define CHECK(x) do {if(!(x)){fprintf(stderr,"FAIL %s:%d %s\n",__FILE__,__LINE__,#x);return 1;}}while(0)
int main(void){
    uint32_t storage[2048];
    memset(storage,0xa5,sizeof storage);
    struct port_ini_phvswap v={0};
    const struct port_ini_phvswap_settings s={
        .balance_set_count=11u,.system_pix=32u,
        .max_working_set=64u,.phvec_dyn_type=0x1211u,
        .swapmap_dyn_type=0x3433u};
    CHECK(port_ini_phvswap_init(storage,24u,&s,&v)==PORT_INI_PHVSWAP_NOMEM);
    CHECK(storage[0]==0xa5a5a5a5u && !v.phv_pix_base);
    CHECK(port_ini_phvswap_init(storage,sizeof storage,&s,&v)==PORT_INI_PHVSWAP_OK);
    CHECK(v.phv_slots==12u && v.phv_record_bytes==60u);
    CHECK(v.swapmap_record_bytes==272u && v.pool_bytes==332u);
    const uint8_t *b=(const uint8_t *)storage;
    CHECK(b[8u]==60u && b[10u]==0x11u && b[11u]==0x12u);
    CHECK(v.phv_pix_base==b+12u && v.phv_refcount_base==b+36u);
    CHECK(v.swapper_p0br==b+72u);
    CHECK(b[68u]==0x10u && b[69u]==1u && b[70u]==0x33u && b[71u]==0x34u);
    CHECK(v.swapper_p0lr==64u);
    for(uint32_t i=0;i<11u;i++){
        CHECK(port_ini_phvswap_pix(&v,i)==0u);
        CHECK(port_ini_phvswap_refcount(&v,i)==-1);
    }
    CHECK(port_ini_phvswap_pix(&v,11u)==32u);
    CHECK(port_ini_phvswap_refcount(&v,11u)==1024);
    for(uint32_t i=0;i<v.swapper_p0lr;i++)CHECK(port_ini_phvswap_pte(&v,i)==0u);
    CHECK(port_ini_phvswap_pix(&v,12u)==UINT16_MAX);
    CHECK(port_ini_phvswap_refcount(&v,12u)==INT16_MIN);
    CHECK(port_ini_phvswap_pte(&v,64u)==UINT32_MAX);
    CHECK(b[v.pool_bytes]==0xa5u); /* no write past allocated storage */
    struct port_ini_phvswap_settings bad=s;
    bad.balance_set_count=UINT32_MAX;
    CHECK(port_ini_phvswap_init(storage,sizeof storage,&bad,&v)==PORT_INI_PHVSWAP_BADPARAM);
    bad=s;bad.max_working_set=0u;
    CHECK(port_ini_phvswap_init(storage,sizeof storage,&bad,&v)==PORT_INI_PHVSWAP_BADPARAM);
    bad=s;bad.system_pix=UINT16_MAX+1u;
    CHECK(port_ini_phvswap_init(storage,sizeof storage,&bad,&v)==PORT_INI_PHVSWAP_BADPARAM);
    bad=s;bad.max_working_set=UINT32_MAX;
    CHECK(port_ini_phvswap_init(storage,sizeof storage,&bad,&v)==PORT_INI_PHVSWAP_BADPARAM);
    bad=s;bad.balance_set_count=17000u;
    CHECK(port_ini_phvswap_init(storage,sizeof storage,&bad,&v)==PORT_INI_PHVSWAP_BADPARAM);
    bad=s;bad.max_working_set=16370u;
    CHECK(port_ini_phvswap_init(storage,1024u,&bad,&v)==PORT_INI_PHVSWAP_NOMEM);
    CHECK(port_ini_phvswap_refcount(&v,11u)==1024); /* bad inputs do not mutate */
    bad=s;bad.balance_set_count=0u;bad.max_working_set=1u;
    CHECK(port_ini_phvswap_init(storage,sizeof storage,&bad,&v)==PORT_INI_PHVSWAP_OK);
    CHECK(v.phv_slots==1u && port_ini_phvswap_pix(&v,0u)==32u);
    CHECK(port_ini_phvswap_refcount(&v,0u)==1024);
    CHECK(port_ini_phvswap_pte(&v,0u)==0u);
    puts("INIT.MAR native process header vector, system reference and swap PTE map PASS");
    return 0;
}
