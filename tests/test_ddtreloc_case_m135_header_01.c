#include "ddtreloc_test_common.h"
/* Independent M135 IOC$RELOC_DDT source-record boundary test. */
int test_ddtreloc_m135_header_01(void){
    uint8_t b[176];ddtreloc_fixture(b,sizeof b);
    struct port_ddtreloc_image im={b,sizeof b,0x80010000u};
    const uint32_t old=port_ddtreloc_read32(b+4u);
    REQUIRE(port_ddtreloc_apply(&im)==PORT_DDTRELOC_OK);
    REQUIRE(port_ddtreloc_read32(b+4u)==0x80010000u+old);
    return 0;
}
