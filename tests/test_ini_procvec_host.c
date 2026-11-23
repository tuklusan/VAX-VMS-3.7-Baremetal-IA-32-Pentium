#include "vms_ini_procvec.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"FAIL %s:%d %s\n",__FILE__,__LINE__,#x);return 1;}}while(0)
int main(void){
    uint32_t memory[1024];
    memset(memory,0xcc,sizeof memory);
    struct port_ini_procvec v={0};
    struct port_ini_proc_settings s={
        .max_processes=32,.system_pcb_address=0x328000u,
        .null_pcb_address=0x328100u,.dyn_pcbvec_type=0x473bu};
    CHECK(port_ini_procvec_init(memory,16u,&s,&v)==PORT_INI_PROC_NOMEM);
    CHECK(memory[0]==0xccccccccu && !v.pcb_vector);
    CHECK(port_ini_procvec_init(memory,sizeof memory,&s,&v)==PORT_INI_PROC_OK);
    CHECK(v.allocated_slots==33u && v.allocated_bytes==210u);
    CHECK(v.system_pix==32u && v.max_pix==31u);
    CHECK(((uint8_t *)memory)[8]==210u && ((uint8_t *)memory)[10]==0x3bu);
    CHECK(port_ini_procvec_ptr(&v,0u)==0x328100u);
    CHECK(port_ini_procvec_ptr(&v,31u)==0x328100u);
    CHECK(port_ini_procvec_ptr(&v,32u)==0x328000u);
    CHECK(port_ini_procvec_ptr(&v,33u)==0u);
    for(unsigned i=0;i<=32u;i++)CHECK(port_ini_procvec_seq(&v,i)==0u);
    CHECK(port_ini_procvec_seq(&v,33u)==UINT16_MAX);
    s.max_processes=0u;
    CHECK(port_ini_procvec_init(memory,sizeof memory,&s,&v)==PORT_INI_PROC_BADPARAM);
    s.max_processes=8192u;
    CHECK(port_ini_procvec_init(memory,sizeof memory,&s,&v)==PORT_INI_PROC_BADPARAM);
    s.max_processes=32u;s.null_pcb_address=0u;
    CHECK(port_ini_procvec_init(memory,sizeof memory,&s,&v)==PORT_INI_PROC_BADPARAM);
    CHECK(port_ini_procvec_ptr(&v,0u)==0x328100u); /* failed writes nonmutating */
    puts("INIT.MAR original PCB vector, SYSTEM PIX, NULLPCB and sequence fields PASS");
    return 0;
}
