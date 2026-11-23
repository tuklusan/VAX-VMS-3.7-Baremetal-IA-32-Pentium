#include "vms_ini_syslog.h"
#include <stdint.h>
static uint16_t getword(const uint8_t *p){
    return (uint16_t)p[0] | (uint16_t)((uint16_t)p[1]<<8);
}
uint32_t port_ini_syslog_init(uint8_t *pool,size_t cap,
    const uint8_t *disklog,size_t disk_available,
    const uint8_t *syslog,size_t sys_available,
    const struct port_ini_syslog_settings *settings,
    struct port_ini_syslog_result *out){
    if(!pool||!disklog||!syslog||!settings||!out ||
       settings->size_field_offset>UINT16_MAX-2u ||
       !settings->pool_address ||
       !settings->allocation_alignment ||
       (settings->allocation_alignment & (settings->allocation_alignment-1u)) ||
       settings->allocation_alignment>4096u ||
       (settings->pool_address&(settings->allocation_alignment-1u)) ||
       (uintptr_t)pool & (settings->allocation_alignment-1u))
        return PORT_INI_SYSLOG_BADPARAM;
    uint32_t offset=settings->size_field_offset;
    if(disk_available<(uint64_t)offset+2u ||
       sys_available<(uint64_t)offset+2u)return PORT_INI_SYSLOG_BADPARAM;
    uint16_t disk_bytes=getword(disklog+offset);
    uint16_t sys_bytes=getword(syslog+offset);
    if(disk_bytes<offset+2u || sys_bytes<offset+2u ||
       disk_bytes>disk_available || sys_bytes>sys_available)
        return PORT_INI_SYSLOG_BADPARAM;
    uint32_t align=settings->allocation_alignment;
    uint64_t second=((uint64_t)disk_bytes+align-1u)&~(uint64_t)(align-1u);
    uint64_t all=second+sys_bytes;
    if(all>cap || all>UINT32_MAX ||
       all>(uint64_t)UINT32_MAX-settings->pool_address)
        return PORT_INI_SYSLOG_NOMEM;
    /* No pool mutation before size, alignment, capacity and address checks.
     * The two sources are immutable prelinked image data, not pool aliases. */
    uintptr_t pool_base=(uintptr_t)pool;
    if(((uintptr_t)disklog>=pool_base &&
        (uintptr_t)disklog-pool_base<cap) ||
       ((uintptr_t)syslog>=pool_base &&
        (uintptr_t)syslog-pool_base<cap))
        return PORT_INI_SYSLOG_BADPARAM;
    for(uint32_t i=0;i<disk_bytes;i++)pool[i]=disklog[i];
    for(uint32_t i=disk_bytes;i<(uint32_t)second;i++)pool[i]=0u;
    for(uint32_t i=0;i<sys_bytes;i++)pool[second+i]=syslog[i];
    *out=(struct port_ini_syslog_result){
        .logical_table={settings->pool_address,
                        settings->pool_address+(uint32_t)second},
        .records={pool,pool+second},
        .sizes={disk_bytes,sys_bytes},
        .consumed_bytes=(uint32_t)all};
    return PORT_INI_SYSLOG_OK;
}
