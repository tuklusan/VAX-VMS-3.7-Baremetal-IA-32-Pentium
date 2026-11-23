#include "ddtreloc_test_common.h"
/* Independent M135 IOC$RELOC_DDT source-record boundary test. */
int test_ddtreloc_m135_fdt_mask_03(void){
    uint8_t b[176];ddtreloc_fixture(b,sizeof b);
    struct port_ddtreloc_image im={b,sizeof b,0x80010000u};
    port_ddtreloc_write32(b+52u,0x12340000u+3u);
    REQUIRE(port_ddtreloc_apply(&im)==PORT_DDTRELOC_OK);
    REQUIRE(port_ddtreloc_read32(b+52u)==0x12340000u+3u);
    return 0;
}
