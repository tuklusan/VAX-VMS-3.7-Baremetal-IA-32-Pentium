#include "vms_log_system.h"
/* Publish two FIN_DATABASE records in a private read-only SYSTEM table.
 * This is only original initial-name lookup, NOT $CRELNM or global LOG$HASH
 * linkage; real scheduler/mutex/group/process table services remain absent. */
uint32_t port_log_system_init(struct port_log_system_table *t,
    uint8_t *disk,uint8_t *sysdev,size_t bytes,unsigned hash_bits){
    if(hash_bits>PORT_LOG_BITS||
       port_log_system_validate(t,disk,sysdev,bytes)!=PORT_LOG_OK)
        return PORT_LOG_BAD;
    const uint32_t a=(uint32_t)(uintptr_t)disk,b=(uint32_t)(uintptr_t)sysdev;
    if(!a||!b||a==b)return PORT_LOG_BAD;
    const unsigned ah=port_log_hash(disk+21u,8u,hash_bits);
    const unsigned bh=port_log_hash(sysdev+21u,13u,hash_bits);
    if(ah>=PORT_LOG_SLOTS||bh>=PORT_LOG_SLOTS)return PORT_LOG_BAD;
    /* No address truncation ambiguity with the two source LOG records. */
    for(unsigned i=0u;i<(1u<<hash_bits);i++){
        const uint32_t head=(uint32_t)(uintptr_t)&t->head[i];
        if(!head||head==a||head==b)return PORT_LOG_BAD;
    }
    t->record_addr[0]=a;t->record_addr[1]=b;
    t->records[0]=disk;t->records[1]=sysdev;
    t->hash_bits=(uint8_t)hash_bits;
    /* Publish the bounded address map before collision insertion walks it. */
    t->count=2u;
    if(port_log_system_insert(t,0u)!=PORT_LOG_OK||
       port_log_system_insert(t,1u)!=PORT_LOG_OK){t->count=0u;return PORT_LOG_CYCLE;}
    t->ready=1u;
    if(port_log_system_check(t)!=PORT_LOG_OK){
        t->ready=0u;return PORT_LOG_CYCLE;
    }
    return PORT_LOG_OK;
}
