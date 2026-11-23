#include "ddtreloc_test_common.h"
int test_ddtreloc_action_offsets(void){
    uint8_t b[176];ddtreloc_fixture(b,sizeof b);struct port_ddtreloc_image im={b,sizeof b,0x80010000u};REQUIRE(port_ddtreloc_apply(&im)==PORT_DDTRELOC_OK);for(unsigned i=0;i<10u;i++){uint32_t at=64u+12u*i;REQUIRE(port_ddtreloc_read32(b+at)==0x80010000u+at-8u+0x120u+i);REQUIRE(port_ddtreloc_read32(b+at-8u)==0u);}
    return 0;
}
