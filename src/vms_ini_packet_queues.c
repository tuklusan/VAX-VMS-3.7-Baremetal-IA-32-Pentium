#include "vms_ini_packet_queues.h"
#include <stddef.h>
static void put32(uint8_t *p,uint32_t n){
    p[0]=(uint8_t)n;p[1]=(uint8_t)(n>>8);
    p[2]=(uint8_t)(n>>16);p[3]=(uint8_t)(n>>24);
}
static void put16(uint8_t *p,uint16_t n){p[0]=(uint8_t)n;p[1]=(uint8_t)(n>>8);}
uint32_t port_ini_packet_init(const struct port_ini_packet_area *a,
                             struct port_ini_packet_list *out){
    if(!a||!out || !a->sentinel_address || (a->sentinel_address&3u) ||
       (a->address&15u) || !a->packet_bytes ||
       (a->packet_bytes&15u) || a->packet_bytes<16u ||
       a->packet_bytes>UINT16_MAX ||
       a->minimum_bytes>a->packet_bytes ||
       (a->count && !a->memory))return PORT_INI_PACKETS_BADPARAM;
    uint64_t used=(uint64_t)a->count*a->packet_bytes;
    uint64_t end=(uint64_t)a->address+used;
    if(used>a->available_bytes || end>UINT32_MAX ||
       (a->count && ((uint64_t)a->sentinel_address>=a->address &&
        (uint64_t)a->sentinel_address<end)))return PORT_INI_PACKETS_BADPARAM;
    uint32_t remainder=(a->count && (end&511u))?
         512u-(uint32_t)(end&511u):0u;
    /* For the partial-page tail original INIT writes a longword containing
     * the space left in its allocated page. It is not another packet. */
    if(used+remainder>a->available_bytes ||
       (uint64_t)a->address+used+remainder>UINT32_MAX)return PORT_INI_PACKETS_BADPARAM;
    struct port_ini_packet_list tmp={0};
    tmp.next=tmp.previous=a->sentinel_address;
    tmp.packet_count=a->count;
    tmp.packet_bytes=a->packet_bytes;
    tmp.minimum_bytes=a->minimum_bytes;
    tmp.split_address=a->address;
    /* INSQUE (R0),@IOC$GL_IRPBL: insert at head in increasing addresses.
     * New head points to old head; old head's previous points to new. */
    for(uint32_t i=0u;i<a->count;i++){
        uint32_t at=i*a->packet_bytes;
        uint32_t va=a->address+at;
        put32(a->memory+at,tmp.next);
        put32(a->memory+at+4u,a->sentinel_address);
        put16(a->memory+at+8u,(uint16_t)a->packet_bytes);
        if(i){
            uint32_t old=(i-1u)*a->packet_bytes;
            put32(a->memory+old+4u,va);
        } else tmp.previous=va;
        tmp.next=va;
    }
    if(remainder){
        uint32_t off=(uint32_t)used;
        put32(a->memory+off,remainder);
        tmp.remainder_address=(uint32_t)end;
        tmp.remainder_bytes=remainder;
    }
    *out=tmp;
    return PORT_INI_PACKETS_OK;
}
uint32_t port_ini_packet_first(const struct port_ini_packet_list *list,
                               uint32_t sentinel_address){
    return list && list->next!=sentinel_address?list->next:0u;
}
