#include "ddtreloc_test_common.h"
int test_ddtreloc_once_only(void){
    uint8_t b[176];ddtreloc_fixture(b,sizeof b);struct port_ddtreloc_image im={b,sizeof b,0x80010000u};REQUIRE(port_ddtreloc_apply(&im)==PORT_DDTRELOC_OK);uint8_t prior[176];memcpy(prior,b,176);REQUIRE(port_ddtreloc_apply(&im)==PORT_DDTRELOC_OK);REQUIRE(!memcmp(prior,b,176));
    return 0;
}
