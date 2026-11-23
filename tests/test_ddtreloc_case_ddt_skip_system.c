#include "ddtreloc_test_common.h"
int test_ddtreloc_ddt_skip_system(void){
    uint8_t b[176];ddtreloc_fixture(b,sizeof b);port_ddtreloc_write32(b+0u,0x81234567u);struct port_ddtreloc_image im={b,sizeof b,0x80010000u};REQUIRE(port_ddtreloc_apply(&im)==PORT_DDTRELOC_OK);REQUIRE(port_ddtreloc_read32(b)==0x81234567u);
    return 0;
}
