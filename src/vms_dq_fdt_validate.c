#include "vms_dq_fdt.h"
/* Fail closed on corruption: DQ masks cannot become an unverified live FDT. */
uint32_t port_dq_fdt_validate(const struct port_dq_fdt *p,size_t bytes){
    if(!p||bytes<sizeof(*p)||((uintptr_t)p&7u))return PORT_DQ_FDT_BAD;
    const uint64_t expected[PORT_DQ_FDT_ROWS]={
      port_dq_fdt_legal(),port_dq_fdt_buffered(),port_dq_fdt_read(),
      port_dq_fdt_write(),port_dq_fdt_access(),port_dq_fdt_deaccess(),
      port_dq_fdt_modify(),port_dq_fdt_mount(),port_dq_fdt_zero(),
      port_dq_fdt_one(),port_dq_fdt_sense(),port_dq_fdt_set()};
    for(unsigned i=0;i<PORT_DQ_FDT_ROWS;i++)
        if(p->mask[i]!=expected[i])return PORT_DQ_FDT_BAD;
    return PORT_DQ_FDT_OK;
}
