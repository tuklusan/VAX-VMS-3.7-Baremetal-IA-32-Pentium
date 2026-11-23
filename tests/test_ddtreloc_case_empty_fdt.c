#include "ddtreloc_test_common.h"
int test_ddtreloc_empty_fdt(void){
    uint8_t b[176];ddtreloc_fixture(b,sizeof b);b[36]=0;struct port_ddtreloc_image im={b,sizeof b,0x80010000u};uint8_t old[176];memcpy(old,b,sizeof b);REQUIRE(port_ddtreloc_apply(&im)==PORT_DDTRELOC_UNBOUND);REQUIRE(!memcmp(old,b,sizeof b));
    return 0;
}
