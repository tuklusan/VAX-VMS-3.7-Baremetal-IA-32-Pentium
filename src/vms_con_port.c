#include "vms_con_port.h"
#include "vms_v37_dyn_tags.h"
/* Source little-endian UCB layout and TTY UCB extension offsets.
 * Caller must supply actual IA-32 resident data or a host-test copy.
 * No reinterpreted host-endian structs, no VAX interpreter. */
uint16_t port_v37_con_get16(const uint8_t *u,unsigned n){
    return (uint16_t)((unsigned)u[n]|((unsigned)u[n+1u]<<8));
}
uint32_t port_v37_con_get32(const uint8_t *u,unsigned n){
    return (uint32_t)u[n]|((uint32_t)u[n+1u]<<8)|
        ((uint32_t)u[n+2u]<<16)|((uint32_t)u[n+3u]<<24);
}
void port_v37_con_set16(uint8_t *u,unsigned n,uint16_t x){
    u[n]=(uint8_t)x;u[n+1u]=(uint8_t)(x>>8);
}
void port_v37_con_set32(uint8_t *u,unsigned n,uint32_t x){
    u[n]=(uint8_t)x;u[n+1u]=(uint8_t)(x>>8);
    u[n+2u]=(uint8_t)(x>>16);u[n+3u]=(uint8_t)(x>>24);
}
uint32_t port_v37_con_valid(const uint8_t *u,size_t capacity){
    if(!u || capacity<PORT_V37_CON_UCB_BYTES ||
       ((uintptr_t)u&3u))return PORT_V37_CON_BAD;
    const uint32_t declared=port_v37_con_get16(u+0u,8u);
    return u[10u]==PORT_V37_DYN_UCB &&
           declared>=PORT_V37_CON_UCB_BYTES && declared<=capacity ?
           PORT_V37_CON_OK:PORT_V37_CON_BAD;
}
uint32_t port_v37_con_kick(uint8_t *u,const struct port_v37_con_backend *b){
    uint16_t status=port_v37_con_get16(u,PORT_V37_CON_STS);
    if(status&PORT_V37_CON_INT)return PORT_V37_CON_OK;
    /* The original CON$XON / CON$RESUME calls TERMINALIO here.
     * Without its real class/port backend leave output queued, not sent. */
    if(!b || !b->kick || b->kick(b->context)!=PORT_V37_CON_OK)
        return PORT_V37_CON_DEFERRED;
    port_v37_con_set16(u,PORT_V37_CON_STS,(uint16_t)(status|PORT_V37_CON_INT));
    return PORT_V37_CON_OK;
}
