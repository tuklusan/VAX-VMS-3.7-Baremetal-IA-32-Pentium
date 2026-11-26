#include "vms_log_system.h"
/* LOGNAMSUB.MAR LOG$HASH, lines 275-303: VAX little-endian XOR longword,
 * ROTL 9 for full words, ROTL 13 for remaining bytes, MULL 0x71279461,
 * ASHQ obtains high hash_bits of low-32-bit multiplication product. */
static uint32_t rotate(uint32_t x,unsigned n){return (x<<n)|(x>>(32u-n));}
uint32_t port_log_hash(const uint8_t *name,size_t length,unsigned bits){
    if(!name||!length||length>PORT_LOG_MAX_NAME||bits>PORT_LOG_MAX_BITS)
        return UINT32_MAX;
    uint32_t value=(uint32_t)length;
    size_t i=0u;
    for(;i+4u<=length;i+=4u){
        const uint32_t word=(uint32_t)name[i]|
           ((uint32_t)name[i+1u]<<8)|((uint32_t)name[i+2u]<<16)|
           ((uint32_t)name[i+3u]<<24);
        value=rotate(value^word,9u);
    }
    for(;i<length;i++)value=rotate(value^name[i],13u);
    value*=UINT32_C(0x71279461);
    return bits?value>>(32u-bits):0u;
}
