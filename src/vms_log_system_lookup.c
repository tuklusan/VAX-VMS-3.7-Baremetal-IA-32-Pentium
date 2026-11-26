#include "vms_log_system.h"
/* LOG$SEARCHLOG: bounded SYSTEM-table source hash-chain search. The VMS
 * process/group priority and original LOG$LOCKR mutex remain unimplemented. */
uint32_t port_log_system_lookup(const struct port_log_system_table *t,
    const uint8_t *name,size_t n,uint32_t *found){
    if(!t||!t->ready||t->count<2u||t->count>PORT_LOG_RECORDS||!found||!name||
       n==0u||n>PORT_LOG_MAX_NAME)return PORT_LOG_BAD;
    const uint32_t bucket=port_log_hash(name,n,t->hash_bits);
    if(bucket>=PORT_LOG_SLOTS)return PORT_LOG_BAD;
    uint32_t record=t->head[bucket];
    for(unsigned count=0u;record;count++){
        if(count>=t->count)return PORT_LOG_CYCLE;
        uint8_t *r=port_log_source_record(t,record);
        if(!r||r[10u]!=PORT_V37_DYN_LOG||r[11u]!=PORT_LOG_SYSTEM||
           r[20u]>PORT_LOG_MAX_NAME||
           port_v37_read16(r+8u)<PORT_LOG_REC_LEN||
           port_v37_read16(r+8u)>PORT_LOG_MAX_REC_LEN||
           22u+(size_t)r[20u]>port_v37_read16(r+8u)||
           23u+(size_t)r[20u]+r[21u+r[20u]]>
               port_v37_read16(r+8u))return PORT_LOG_CYCLE;
        const uint32_t c=port_log_name_compare(r+21u,r[20u],name,n);
        if(c==1u){*found=record;return PORT_LOG_OK;}
        if(c==2u)break;
        record=port_v37_read32(r);
    }
    return PORT_LOG_NOTRAN;
}
