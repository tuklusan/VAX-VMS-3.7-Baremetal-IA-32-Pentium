#include "vms_log_system.h"
/* Independently validate every native bucket and both source record links.
 * No silent cycle, alias, false DDT or system-wide publication permitted. */
uint32_t port_log_system_check(const struct port_log_system_table *t){
    if(!t||!t->ready||t->count!=2u||t->hash_bits>PORT_LOG_BITS||
       !t->records[0]||!t->records[1]||
       t->record_addr[0]==0u||t->record_addr[1]==0u||
       t->record_addr[0]==t->record_addr[1])return PORT_LOG_BAD;
    unsigned seen=0u;
    for(unsigned slot=0u;slot<PORT_LOG_SLOTS;slot++){
        uint32_t last=(uint32_t)(uintptr_t)&t->head[slot];
        uint32_t cur=t->head[slot];
        unsigned steps=0u;
        if(slot>=(1u<<t->hash_bits)&&cur)return PORT_LOG_CYCLE;
        while(cur){
            if(++steps>2u)return PORT_LOG_CYCLE;
            uint8_t *r=port_log_source_record(t,cur);
            if(!r||r[11u]!=PORT_LOG_SYSTEM||r[10u]!=PORT_V37_DYN_LOG||
               port_v37_read16(r+8u)!=PORT_LOG_REC_LEN||
               22u+(size_t)r[20u]>PORT_LOG_REC_LEN||
               port_v37_read32(r+4u)!=last||
               port_log_hash(r+21u,r[20u],t->hash_bits)!=slot)
                return PORT_LOG_CYCLE;
            const unsigned index=(cur==t->record_addr[0])?0u:1u;
            if((seen&(1u<<index))!=0u)return PORT_LOG_CYCLE;
            seen|=(1u<<index);
            last=cur;cur=port_v37_read32(r);
        }
    }
    return seen==3u?PORT_LOG_OK:PORT_LOG_CYCLE;
}
