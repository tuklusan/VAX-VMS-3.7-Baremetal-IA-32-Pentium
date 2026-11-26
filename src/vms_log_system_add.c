#include "vms_log_system.h"

/* SYSINIT's $CRELNM destination is the SYSTEM RSHT initialized by SWAPINIT.
 * This port boundary accepts a caller-built source LOG record and retains
 * its storage owner; allocation and executive mutex semantics are caller
 * responsibilities and are not implied here. */
uint32_t port_log_system_add(struct port_log_system_table *t,uint8_t *record,
                             uint32_t address,size_t bytes){
    if(!t||!t->ready||!record||!address||bytes<PORT_LOG_REC_LEN)
        return PORT_LOG_BAD;
    const size_t rec_len=port_v37_read16(record+8u);
    if(rec_len<PORT_LOG_REC_LEN||
       rec_len>PORT_LOG_MAX_REC_LEN||bytes<rec_len||
       t->count<2u||t->count>=PORT_LOG_RECORDS||
       port_log_system_check(t)!=PORT_LOG_OK||
       record[10u]!=PORT_V37_DYN_LOG||record[11u]!=PORT_LOG_SYSTEM||
       port_v37_read16(record+8u)!=rec_len||
       record[20u]==0u||record[20u]>PORT_LOG_MAX_NAME||
       22u+(size_t)record[20u]>rec_len||
       record[21u+record[20u]]==0u||
       23u+(size_t)record[20u]+record[21u+record[20u]]>rec_len||
       port_v37_read32(record)!=0u||port_v37_read32(record+4u)!=0u)
        return PORT_LOG_BAD;
    for(unsigned i=0u;i<t->count;i++)
        if(address==t->record_addr[i]||record==t->records[i])
            return PORT_LOG_BAD;
    for(unsigned i=0u;i<PORT_LOG_SLOTS;i++)
        if(address==(uint32_t)(uintptr_t)&t->head[i])return PORT_LOG_BAD;
    uint32_t found=0u;
    const uint32_t existing=port_log_system_lookup(t,record+21u,
        record[20u],&found);
    if(existing!=PORT_LOG_NOTRAN)return PORT_LOG_BAD;
    const unsigned idx=t->count;
    t->records[idx]=record;t->record_addr[idx]=address;
    if(port_log_system_insert(t,idx)!=PORT_LOG_OK){
        t->records[idx]=0;t->record_addr[idx]=0;
        return PORT_LOG_CYCLE;
    }
    t->count++;
    if(port_log_system_check(t)!=PORT_LOG_OK)return PORT_LOG_CYCLE;
    return PORT_LOG_OK;
}
