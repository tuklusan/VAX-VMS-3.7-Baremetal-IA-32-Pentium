#include "ddtreloc_test_common.h"
int test_ddtreloc_fdt_skip_system(void){
    uint8_t b[176];ddtreloc_fixture(b,sizeof b);port_ddtreloc_write32(b+64u+36u,0x92345678u);struct port_ddtreloc_image im={b,sizeof b,0x80010000u};REQUIRE(port_ddtreloc_apply(&im)==PORT_DDTRELOC_OK);REQUIRE(port_ddtreloc_read32(b+100u)==0x92345678u);
    return 0;
}
