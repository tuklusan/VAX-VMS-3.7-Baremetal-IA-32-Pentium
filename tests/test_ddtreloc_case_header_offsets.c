#include "ddtreloc_test_common.h"
int test_ddtreloc_header_offsets(void){
    uint8_t b[176];ddtreloc_fixture(b,sizeof b);struct port_ddtreloc_image im={b,sizeof b,0x80010000u};REQUIRE(port_ddtreloc_apply(&im)==PORT_DDTRELOC_OK);const unsigned a[]={0,4,8,12,16,24,28,32};for(unsigned i=0;i<8;i++){uint32_t want=(i==2u)?0x80010028u:0x80010110u+4u*i;REQUIRE(port_ddtreloc_read32(b+a[i])==want);}
    return 0;
}
