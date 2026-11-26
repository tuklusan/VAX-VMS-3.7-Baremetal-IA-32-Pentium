#include "vms_log_system.h"
/* LOG$INSLOGN_LCK: insert in the hash bucket sorted by name length and
 * unsigned lexical order. LOG$L_LTFL and LOG$L_LTBL are original 32-bit
 * physical native links; no process or group lock is fabricated. */
uint32_t port_log_system_insert(struct port_log_system_table *t,unsigned idx){
    if(!t||idx>=5u||!t->records[idx]||!t->record_addr[idx]||
       t->hash_bits>PORT_LOG_BITS)return PORT_LOG_BAD;
    uint8_t *const rec=t->records[idx];const size_t len=rec[20u];
    const uint32_t bucket=port_log_hash(rec+21u,len,t->hash_bits);
    if(bucket>=PORT_LOG_SLOTS)return PORT_LOG_BAD;
    uint32_t *prev=&t->head[bucket];
    uint32_t prev_addr=(uint32_t)(uintptr_t)prev;
    uint32_t next=*prev;
    for(unsigned count=0u;next;count++){
        if(count>=PORT_LOG_RECORDS)return PORT_LOG_CYCLE;
        uint8_t *other=port_log_source_record(t,next);
        if(!other || other==rec)return PORT_LOG_CYCLE;
        if(port_log_name_compare(other+21u,other[20u],rec+21u,len)!=0u)
            break;
        prev_addr=next;
        prev=(uint32_t *)(void *)other;
        next=*prev;
    }
    port_v37_write32(rec,next);
    port_v37_write32(rec+4u,prev_addr);
    if(next){
        uint8_t *other=port_log_source_record(t,next);
        if(!other)return PORT_LOG_CYCLE;
        port_v37_write32(other+4u,t->record_addr[idx]);
    }
    *prev=t->record_addr[idx];
    return PORT_LOG_OK;
}
