#include "vms_log_system.h"
/* Independently validate every native bucket and all live source record links.
 * No silent cycle, alias, false DDT or system-wide publication permitted. */
uint32_t port_log_system_check(const struct port_log_system_table *t){
    if(!t||!t->ready||t->count<2u||t->count>5u||t->hash_bits>PORT_LOG_BITS||
       t->count>PORT_LOG_RECORDS)return PORT_LOG_BAD;
    unsigned seen=0u;
    for(unsigned i=0u;i<t->count;i++){
        if(!t->records[i]||!t->record_addr[i])return PORT_LOG_BAD;
        for(unsigned j=0u;j<i;j++)
            if(t->record_addr[i]==t->record_addr[j]||
               t->records[i]==t->records[j])return PORT_LOG_BAD;
    }
    for(unsigned slot=0u;slot<PORT_LOG_SLOTS;slot++){
        uint32_t last=(uint32_t)(uintptr_t)&t->head[slot];
        uint32_t cur=t->head[slot];
        unsigned steps=0u;
        if(slot>=(1u<<t->hash_bits)&&cur)return PORT_LOG_CYCLE;
        while(cur){
            if(++steps>t->count)return PORT_LOG_CYCLE;
            uint8_t *r=port_log_source_record(t,cur);
            if(!r||r[11u]!=PORT_LOG_SYSTEM||r[10u]!=PORT_V37_DYN_LOG||
               port_v37_read16(r+8u)>PORT_LOG_MAX_REC_LEN||
               r[20u]==0u||r[20u]>PORT_LOG_MAX_NAME||
               22u+(size_t)r[20u]>port_v37_read16(r+8u)||
               23u+(size_t)r[20u]+r[21u+r[20u]]>
                   port_v37_read16(r+8u)||
               port_v37_read32(r+4u)!=last||
               port_log_hash(r+21u,r[20u],t->hash_bits)!=slot)
                return PORT_LOG_CYCLE;
            unsigned index=0u;
            while(index<t->count&&t->record_addr[index]!=cur)index++;
            if(index==t->count)return PORT_LOG_CYCLE;
            if((seen&(1u<<index))!=0u)return PORT_LOG_CYCLE;
            seen|=(1u<<index);
            last=cur;cur=port_v37_read32(r);
        }
    }
    return seen==((1u<<t->count)-1u)?PORT_LOG_OK:PORT_LOG_CYCLE;
}
