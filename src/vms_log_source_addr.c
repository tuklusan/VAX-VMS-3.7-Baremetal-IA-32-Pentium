#include "vms_log_system.h"
/* SYSDEF.MDL stores physical native 32-bit addresses, never host indexes. */
uint32_t port_log_source_address(const struct port_log_system_table *t,
    const uint8_t *r,uint32_t *out){
    if(!t||!r||!out)return PORT_LOG_BAD;
    for(unsigned i=0u;i<2u;i++)
        if(t->records[i]==r&&t->record_addr[i]){
            *out=t->record_addr[i];return PORT_LOG_OK;
        }
    return PORT_LOG_BAD;
}
