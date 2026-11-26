#include "vms_log_system.h"
/* LOGNAMSUB.MAR LOG$TRANSLATE 373-420: a leading underscore bypasses
 * lookup and removes exactly one escape character. Otherwise search
 * source SYSTEM logicals only; process/group tables are absent. The resident
 * SYSTEM table has a bounded native record map. */
uint32_t port_log_translate(const struct port_log_system_table *t,
    const char *text,size_t length,uint32_t disable_mask,
    uint8_t *outbuf,size_t capacity,struct port_log_result *out){
    if(!t||!text||!outbuf||!out||length==0u||
       length>PORT_LOG_MAX_NAME||!t->ready||t->count<2u||t->count>5u||
       port_log_system_check(t)!=PORT_LOG_OK)return PORT_LOG_BAD;
    const uint8_t *name=(const uint8_t *)text;
    if(name[0]=='_')
        return port_log_copy_result(name+1u,length-1u,outbuf,capacity,
                  PORT_LOG_NOTRAN,0u,0u,out);
    if(disable_mask&1u)
        return port_log_copy_result(name,length,outbuf,capacity,
                  PORT_LOG_NOTRAN,0u,0u,out);
    uint32_t address=0u;
    const uint32_t status=port_log_system_lookup(t,name,length,&address);
    if(status==PORT_LOG_NOTRAN)
        return port_log_copy_result(name,length,outbuf,capacity,
                  PORT_LOG_NOTRAN,0u,0u,out);
    if(status!=PORT_LOG_OK)return status;
    const uint8_t *const r=port_log_source_record(t,address);
    if(!r||port_v37_read16(r+8u)>PORT_LOG_MAX_REC_LEN||
       22u+(size_t)r[20u]>port_v37_read16(r+8u))return PORT_LOG_CYCLE;
    const unsigned offset=21u+r[20u];
    const unsigned n=r[offset];
    if(offset+1u+n>port_v37_read16(r+8u))return PORT_LOG_CYCLE;
    return port_log_copy_result(r+offset+1u,n,outbuf,capacity,
                 PORT_LOG_OK,address,r[14u],out);
}
