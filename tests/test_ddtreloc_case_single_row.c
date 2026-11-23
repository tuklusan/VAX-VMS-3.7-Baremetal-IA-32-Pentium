#include "ddtreloc_test_common.h"
int test_ddtreloc_single_row(void){
    uint8_t b[68];ddtreloc_fixture(b,sizeof b);b[36]=28u;struct port_ddtreloc_image im={b,sizeof b,0x80010000u};REQUIRE(port_ddtreloc_apply(&im)==PORT_DDTRELOC_OK);REQUIRE(port_ddtreloc_read32(b+64u)==0x80010158u);
    return 0;
}
