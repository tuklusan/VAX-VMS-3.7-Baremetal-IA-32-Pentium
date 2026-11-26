#include "vms_sysinit_names.h"

static uint32_t table_translate(void *opaque,const char *name,char *buffer,
                                unsigned capacity,unsigned *received){
    struct port_sip_log_table_context *c=opaque;
    struct port_log_result result={0};
    if(!c||!c->table||!name||!buffer||!received)return PORT_SIP_BADARG;
    size_t n=0u;while(n<=PORT_LOG_MAX_NAME&&name[n])n++;
    if(!n||n>PORT_LOG_MAX_NAME)return PORT_SIP_BADARG;
    const uint32_t status=port_log_translate(c->table,name,n,0u,
        (uint8_t *)buffer,capacity,&result);
    if(status!=PORT_LOG_OK)return status;
    *received=result.bytes;return PORT_SIP_OK;
}

static uint32_t table_create(void *opaque,const char *name,const char *value){
    struct port_sip_log_table_context *c=opaque;
    if(!c||!c->table||!c->records||!c->record_addresses||
       c->created>=PORT_SIP_NAME_COUNT||!name||!value)return PORT_SIP_BADARG;
    size_t nn=0u,vn=0u;
    while(nn<=PORT_LOG_MAX_NAME&&name[nn])nn++;
    while(vn<PORT_LOG_MAX_REC_LEN&&value[vn])vn++;
    const size_t length=23u+nn+vn;
    if(!nn||nn>PORT_LOG_MAX_NAME||!vn||length>PORT_LOG_MAX_REC_LEN||
       !c->record_addresses[c->created])return PORT_SIP_TOOLONG;
    uint8_t *r=c->records[c->created];
    for(size_t i=0u;i<PORT_LOG_MAX_REC_LEN;i++)r[i]=0u;
    port_v37_write16(r+8u,(uint16_t)length);
    r[10u]=PORT_V37_DYN_LOG;r[11u]=PORT_LOG_SYSTEM;r[20u]=(uint8_t)nn;
    for(size_t i=0u;i<nn;i++)r[21u+i]=(uint8_t)name[i];
    r[21u+nn]=(uint8_t)vn;
    for(size_t i=0u;i<vn;i++)r[22u+nn+i]=(uint8_t)value[i];
    const uint32_t status=port_log_system_add(c->table,r,
        c->record_addresses[c->created],PORT_LOG_MAX_REC_LEN);
    if(status!=PORT_LOG_OK)return status;
    c->created++;return PORT_SIP_OK;
}

uint32_t port_sip_create_system_names_in_table(
    struct port_log_system_table *table,
    uint8_t records[PORT_SIP_NAME_COUNT][PORT_LOG_MAX_REC_LEN],
    const uint32_t addresses[PORT_SIP_NAME_COUNT],const uint8_t top_system[10]){
    if(!table||!records||!addresses)return PORT_SIP_BADARG;
    struct port_sip_log_table_context context={table,records,addresses,0u};
    const struct port_sip_logical_ops ops={table_translate,table_create};
    const uint32_t status=port_sip_create_system_names(&ops,&context,top_system);
    if(status!=PORT_SIP_OK)return status;
    return context.created==PORT_SIP_NAME_COUNT?PORT_SIP_OK:PORT_SIP_SERVICE_ERROR;
}
