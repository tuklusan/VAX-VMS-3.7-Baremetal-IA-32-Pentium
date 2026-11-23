#include "ddtreloc_test_common.h"
int test_ddtreloc_zero_rows(void){
    uint8_t b[56];ddtreloc_fixture(b,sizeof b);b[36]=16u;struct port_ddtreloc_image im={b,sizeof b,0x80010000u};REQUIRE(port_ddtreloc_apply(&im)==PORT_DDTRELOC_OK);REQUIRE(port_ddtreloc_read32(b+8u)==0x80010028u);
    return 0;
}
