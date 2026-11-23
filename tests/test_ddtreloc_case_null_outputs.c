#include "ddtreloc_test_common.h"
int test_ddtreloc_null_outputs(void){
    uint8_t b[176];ddtreloc_fixture(b,sizeof b);struct port_ddtreloc_image im={b,sizeof b,0x80010000u};uint32_t off=3;uint16_t n=5;REQUIRE(port_ddtreloc_preflight(&im,NULL,&n)==PORT_DDTRELOC_BAD);REQUIRE(port_ddtreloc_preflight(&im,&off,NULL)==PORT_DDTRELOC_BAD);REQUIRE(off==3&&n==5);
    return 0;
}
