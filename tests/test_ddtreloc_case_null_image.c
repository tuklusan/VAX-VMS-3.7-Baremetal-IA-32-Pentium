#include "ddtreloc_test_common.h"
int test_ddtreloc_null_image(void){
    struct port_ddtreloc_image im={0};uint32_t off=5;uint16_t n=7;REQUIRE(port_ddtreloc_preflight(&im,&off,&n)==PORT_DDTRELOC_BAD);REQUIRE(off==5&&n==7);
    return 0;
}
