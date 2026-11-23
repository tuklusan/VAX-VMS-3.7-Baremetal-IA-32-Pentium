#include "ddtreloc_test_common.h"
int test_ddtreloc_word_io(void){
    uint8_t b[8]={0};port_ddtreloc_write32(b+1u,0x12345678u);REQUIRE(b[1]==0x78u&&b[2]==0x56u&&b[3]==0x34u&&b[4]==0x12u);REQUIRE(port_ddtreloc_read32(b+1u)==0x12345678u);REQUIRE(port_ddtreloc_read16(b+1u)==0x5678u);
    return 0;
}
