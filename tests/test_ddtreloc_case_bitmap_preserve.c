#include "ddtreloc_test_common.h"
int test_ddtreloc_bitmap_preserve(void){
    uint8_t b[176];ddtreloc_fixture(b,sizeof b);for(unsigned i=40;i<56;i++)b[i]=(uint8_t)(i*7u);uint8_t prior[16];memcpy(prior,b+40u,16);struct port_ddtreloc_image im={b,sizeof b,0x80010000u};REQUIRE(port_ddtreloc_apply(&im)==PORT_DDTRELOC_OK);REQUIRE(!memcmp(prior,b+40u,16));
    return 0;
}
