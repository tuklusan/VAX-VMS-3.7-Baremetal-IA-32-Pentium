#include "vms_ini_mpw.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"FAIL %s:%d %s\n",__FILE__,__LINE__,#x);return 1;}}while(0)
int main(void){
    uint32_t mem[1024];struct port_ini_mpw out={0};
    struct port_ini_mpw_settings s={.requested_fault_cluster=39u,
                                    .dyn_mpwmap_type=0x734eu};
    memset(mem,0xa5,sizeof mem);
    CHECK(port_ini_mpw_init(mem,245u,&s,&out)==PORT_INI_MPW_NOMEM);
    CHECK(mem[0]==0xa5a5a5a5u && !out.pte_base);
    CHECK(port_ini_mpw_init(mem,sizeof mem,&s,&out)==PORT_INI_MPW_OK);
    CHECK(out.effective_fault_cluster==32u && out.swap_increment==32u);
    CHECK(out.allocated_slots==39u && out.allocated_bytes==246u);
    const uint8_t *p=(const uint8_t *)mem;
    CHECK(p[8]==246u && p[9]==0u && p[10]==0x4eu && p[11]==0x73u);
    CHECK(out.pte_base==p+12u && out.phv_index_base==p+168u);
    for(unsigned i=0;i<out.allocated_slots;i++){
        CHECK(port_ini_mpw_pte(&out,i)==0u);
        CHECK(port_ini_mpw_phv_index(&out,i)==0u);
    }
    CHECK(port_ini_mpw_pte(&out,39u)==UINT32_MAX);
    CHECK(port_ini_mpw_phv_index(&out,39u)==UINT16_MAX);
    s.requested_fault_cluster=0;
    CHECK(port_ini_mpw_init(mem,sizeof mem,&s,&out)==PORT_INI_MPW_OK);
    CHECK(out.effective_fault_cluster==16u && out.allocated_slots==23u);
    s.requested_fault_cluster=7;
    CHECK(port_ini_mpw_init(mem,sizeof mem,&s,&out)==PORT_INI_MPW_OK);
    CHECK(out.effective_fault_cluster==16u);
    s.requested_fault_cluster=11000;
    CHECK(port_ini_mpw_init(mem,sizeof mem,&s,&out)==PORT_INI_MPW_BADPARAM);
    s.requested_fault_cluster=UINT32_MAX;
    CHECK(port_ini_mpw_init(mem,sizeof mem,&s,&out)==PORT_INI_MPW_BADPARAM);
    s.requested_fault_cluster=32;
    CHECK(port_ini_mpw_init((uint8_t *)mem+1,sizeof mem-1,&s,&out)==PORT_INI_MPW_BADPARAM);
    CHECK(port_ini_mpw_pte(&out,0)==0u); /* Reject did not reset valid data. */
    puts("INIT.MAR INI_MPW original PTE/PHV index allocation PASS");
    return 0;
}
