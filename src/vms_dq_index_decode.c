#include "vms_dq_index_header.h"
uint16_t port_dq_index_word(const uint8_t *b,size_t p){
    return (uint16_t)(b[p]|(uint16_t)((uint16_t)b[p+1u]<<8));
}
uint32_t port_dq_index_long(const uint8_t *b,size_t p){
    return (uint32_t)port_dq_index_word(b,p) |
           ((uint32_t)port_dq_index_word(b,p+2u)<<16);
}
int port_dq_index_checksum(const uint8_t b[512],size_t at){
    if(!b||at>510u||(at&1u))return 0;
    uint16_t s=0u;
    for(size_t i=0;i<at;i+=2u)s=(uint16_t)(s+port_dq_index_word(b,i));
    return s==port_dq_index_word(b,at);
}
