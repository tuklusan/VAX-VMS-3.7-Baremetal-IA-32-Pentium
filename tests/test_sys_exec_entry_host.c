#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "vms_sys_executive_entry.h"
#include "vms_sysloavec.h"
#define CHECK(x) do{if(!(x)){fprintf(stderr,"FAIL %s:%d %s\n",__FILE__,__LINE__,#x);return 1;}}while(0)
static void u32(uint8_t *p,uint32_t n){
    for(unsigned j=0;j<4;j++)p[j]=(uint8_t)(n>>(8u*j));
}
/* A source-type table lives inside one simulated loadable image. It is
 * never invoked on the host; the real CPU vector dispatch is guest-only. */
static void image(uint8_t b[256]){
    memset(b,0,256u);u32(b,256u);b[8u]=0u;b[9u]=1u;
    b[10u]=PORT_SYSLOA386_DYN_LOADCODE;
    unsigned pos=12u;
    for(unsigned i=0;i<PORT_SYSLOA_COUNT;i++){
        b[pos]=(uint8_t)(i<7u?2u:i<22u?3u:1u);
        u32(b+pos+1u,200u-(pos+1u));
        pos+=5u;
    }
    u32(b+pos,UINT32_MAX);
}
int main(void){
    struct port_sys_exec_result result;
    memset(&result,0xa5,sizeof result);
    struct port_sys_exec_start req={
        .signature=PORT_SYS_EXEC_SIGNATURE,
        .version=PORT_SYS_EXEC_VERSION,
        .system_time=UINT64_C(53312345670000000),
        .pcb_type=PORT_SYS_EXEC_PCB_KIND,
        .initialized_images=3u,.reserved=0u,.result=&result,
        .phase=PORT_SYS_EXEC_INIT_RESIDENT};
    CHECK(port_sys_executive_entry(0)==PORT_SYS_EXEC_BAD_PARAMETERS);
    CHECK(port_sys_executive_entry(&req)==PORT_SYS_EXEC_BAD_PARAMETERS);
    CHECK(result.signature==0xa5a5a5a5u);
    req.signature=0u;
    CHECK(port_sys_executive_entry(&req)==PORT_SYS_EXEC_BAD_PARAMETERS);
    req.signature=PORT_SYS_EXEC_SIGNATURE;req.result=0;
    CHECK(port_sys_executive_entry(&req)==PORT_SYS_EXEC_BAD_PARAMETERS);
    req.result=&result;req.phase=PORT_SYS_EXEC_LINK_SYSLOA;
    CHECK(port_sys_executive_entry(&req)==PORT_SYS_EXEC_BAD_PARAMETERS);
    uint8_t blob[256];image(blob);blob[10u]=0u;
    req.sysloa_descriptor=blob;req.sysloa_bytes=sizeof blob;
    CHECK(port_sys_executive_entry(&req)==PORT_SYS_EXEC_BAD_PARAMETERS);
    CHECK(result.signature==0xa5a5a5a5u);
    image(blob);
    CHECK(port_sys_executive_entry(&req)==PORT_SYS_EXEC_NORMAL);
    CHECK(result.signature==PORT_SYS_EXEC_LINK_RESULT_TAG);
    CHECK(result.sysloa_linked==1u);
    req.phase=PORT_SYS_EXEC_INIT_RESIDENT;req.initialized_images=0u;
    CHECK(port_sys_executive_entry(&req)==PORT_SYS_EXEC_BAD_PARAMETERS);
    req.initialized_images=3u;
    CHECK(port_sys_executive_entry(&req)==PORT_SYS_EXEC_BAD_PARAMETERS);
    CHECK(result.signature==PORT_SYS_EXEC_LINK_RESULT_TAG);
    req.phase=PORT_SYS_EXEC_INIT_PFLVEC;
    struct port_ini_pflvec_settings pf={.page_file_count=1u};
    req.pfl_settings=&pf;
    CHECK(port_sys_executive_entry(&req)==PORT_SYS_EXEC_BAD_PARAMETERS);
    CHECK(result.signature==PORT_SYS_EXEC_LINK_RESULT_TAG);
    req.pfl_settings=0;
    CHECK(port_sys_executive_entry(&req)==PORT_SYS_EXEC_BAD_PARAMETERS);
    req.phase=PORT_SYS_EXEC_INIT_INTSTKLM;
    struct port_sys_exec_intstack_settings live={0x310000u,8u,0u};
    req.intstack_settings=&live;
    CHECK(port_sys_executive_entry(&req)==PORT_SYS_EXEC_BAD_PARAMETERS);
    CHECK(result.signature==PORT_SYS_EXEC_LINK_RESULT_TAG);
    req.intstack_settings=0;
    CHECK(port_sys_executive_entry(&req)==PORT_SYS_EXEC_BAD_PARAMETERS);
    CHECK(result.signature==PORT_SYS_EXEC_LINK_RESULT_TAG);
    req.phase=PORT_SYS_EXEC_INIT_SYSWCB;
    struct port_sys_exec_wcb_settings wcb={0};
    req.syswcb_settings=&wcb;
    CHECK(port_sys_executive_entry(&req)==PORT_SYS_EXEC_BAD_PARAMETERS);
    CHECK(result.signature==PORT_SYS_EXEC_LINK_RESULT_TAG);
    /* Hardware CPU and ATA callbacks are deliberately unavailable in this
     * host test; NULL result pointers remain uninitialized on failure. */
    puts("SYS.EXE resident SYSLOA vector link and fail-closed INIT checks PASS");
    return 0;
}
