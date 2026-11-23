#include "vms_ini_pflvec.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"FAIL %s:%d %s\n",__FILE__,__LINE__,#x);return 1;}}while(0)
int main(void){
    uint32_t memory[512];memset(memory,0xa5,sizeof memory);
    struct port_ini_pflvec_settings s={.page_file_count=4,.swap_file_count=2,
        .null_pfl_address=0x320080u,.ptr_dyn_type=7u,.pfl_dyn_type=9u,
        .global_page_file_limit=500u,.shell_base_address=0x28000u,
        .shell_system_window=0x3200a0u};
    struct port_ini_pflvec p={0};
    CHECK(port_ini_pflvec_init(memory,8u,&s,&p)==PORT_INI_PFL_NOMEM);
    CHECK(memory[0]==0xa5a5a5a5u&&p.pointers==0);
    CHECK(port_ini_pflvec_init(memory,sizeof memory,&s,&p)==PORT_INI_PFL_OK);
    CHECK(p.entries==6u&&p.allocated_bytes==36u);
    const uint8_t *b=(const uint8_t *)memory;
    CHECK(b[4u]==6u&&b[8u]==36u&&b[10u]==7u&&b[11u]==9u);
    CHECK(p.pointers==b+12u);
    for(uint32_t i=0;i<6u;i++)CHECK(port_ini_pflvec_entry(&p,i)==0x320080u);
    CHECK(port_ini_pflvec_entry(&p,6u)==0u);
    CHECK(p.null_pagefile_vbn==(0x28000u/512u+2u));
    CHECK(p.null_pagefile_window==0x3200a0u&&p.actual_swapfile_slots==3u);
    CHECK(p.global_page_file_limit==500u&&b[36u]==0xa5u);
    s.page_file_count=0;s.swap_file_count=0;
    CHECK(port_ini_pflvec_init(memory,sizeof memory,&s,&p)==PORT_INI_PFL_BADPARAM);
    s.page_file_count=UINT16_MAX;s.swap_file_count=UINT16_MAX;
    CHECK(port_ini_pflvec_init(memory,sizeof memory,&s,&p)==PORT_INI_PFL_BADPARAM);
    s.page_file_count=1;s.swap_file_count=1;s.shell_base_address=0x28100u;
    CHECK(port_ini_pflvec_init(memory,sizeof memory,&s,&p)==PORT_INI_PFL_BADPARAM);
    s.shell_base_address=0x28000u;
    CHECK(port_ini_pflvec_init(memory,sizeof memory,&s,&p)==PORT_INI_PFL_OK);
    s.shell_base_address=0u;s.shell_system_window=0u;
    CHECK(port_ini_pflvec_init(memory,sizeof memory,&s,&p)==PORT_INI_PFL_OK);
    CHECK(p.null_pagefile_vbn==0u && p.null_pagefile_window==0u);
    s.shell_system_window=0x3200a0u;
    CHECK(port_ini_pflvec_init(memory,sizeof memory,&s,&p)==PORT_INI_PFL_BADPARAM);
    s.shell_base_address=0x28000u;s.shell_system_window=0u;
    CHECK(port_ini_pflvec_init(memory,sizeof memory,&s,&p)==PORT_INI_PFL_BADPARAM);
    struct port_ini_interrupt_stack stk={0};
    CHECK(port_ini_interrupt_stack_limits(0x310000u,4u,0u,&stk)==PORT_INI_PFL_OK);
    CHECK(stk.low==0x30f800u&&stk.high==0x310000u&&stk.max_resource_depth==128u);
    CHECK(port_ini_interrupt_stack_limits(0x310000u,4u,2048u,&stk)==PORT_INI_PFL_OK);
    CHECK(stk.max_resource_depth==4u);
    CHECK(port_ini_interrupt_stack_limits(0x310000u,40u,0u,&stk)==PORT_INI_PFL_OK);
    CHECK(stk.max_resource_depth==255u);
    CHECK(port_ini_interrupt_stack_limits(100u,1u,0u,&stk)==PORT_INI_PFL_BADPARAM);
    CHECK(port_ini_interrupt_stack_limits(1000u,0u,0u,&stk)==PORT_INI_PFL_BADPARAM);
    puts("INIT.MAR live page/swap PFL pointer vector and interrupt-stack lock depth PASS");
    return 0;
}
