#include "ddtreloc_test_common.h"
/* Independent M135 IOC$RELOC_DDT source-record boundary test. */
int test_ddtreloc_m135_partial_fdt_03(void){
    uint8_t b[176];ddtreloc_fixture(b,sizeof b);
    struct port_ddtreloc_image im={b,sizeof b,0x80010000u};
    b[36u]=(uint8_t)(136u-11u);
    uint8_t prior[176];memcpy(prior,b,sizeof b);
    REQUIRE(port_ddtreloc_apply(&im)==PORT_DDTRELOC_UNBOUND);
    REQUIRE(memcmp(prior,b,sizeof b)==0);
    return 0;
}
