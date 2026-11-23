#include "ddtreloc_test_common.h"
int test_ddtreloc_bad_base(void){
    uint8_t b[176];ddtreloc_fixture(b,sizeof b);struct port_ddtreloc_image im={b,sizeof b,0};REQUIRE(port_ddtreloc_apply(&im)==PORT_DDTRELOC_BAD);im.pc_base=UINT32_MAX-10u;REQUIRE(port_ddtreloc_apply(&im)==PORT_DDTRELOC_BAD);
    return 0;
}
