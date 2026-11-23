#include "vms_hpwd.h"
static const uint8_t sample_hash[8] = {0x00,0x4a,0xd8,0x22,0x63,0xbb,0xe2,0xb7};
static const uint8_t crc_hash[8] = {0x66,0x25,0x0e,0x0c,0,0,0,0};
uint32_t port_hpwd_selftest(void) {
    uint8_t result[8];
    const uint8_t *pw=(const uint8_t *)"PASSWORD";
    const uint8_t *user=(const uint8_t *)"SYSTEM";
    uint32_t errors=0;
    if (!port_hpwd(result,pw,8,1,1234,user,6)) return 0x80000000u;
    for(unsigned i=0;i<8;i++) if(result[i]!=sample_hash[i]) errors |= 1u;
    if(!port_hpwd_matches(sample_hash,pw,8,1,1234,user,6) ||
       port_hpwd_matches(sample_hash,(const uint8_t *)"BAD",3,1,1234,user,6))
        errors |= 2u;
    if(!port_hpwd(result,pw,8,0,0,user,6)) return 0x80000000u;
    for(unsigned i=0;i<8;i++) if(result[i]!=crc_hash[i]) errors |= 4u;
    if(!port_hpwd(result,pw,8,5,1234,user,6)) return 0x80000000u;
    for(unsigned i=0;i<8;i++) if(result[i]!=sample_hash[i]) errors |= 8u;
    return errors;
}
