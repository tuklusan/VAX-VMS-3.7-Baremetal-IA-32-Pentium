#include "vms_dq_fdt.h"
/* Source DQ_FUNCTABLE has exactly twelve mask rows: the first two
 * are legal/buffered and the last ten dispatch to native ACP/EXE actions
 * not yet ported for this disk. Only masks are staged, never an active FDT. */
uint32_t port_dq_fdt_build(struct port_dq_fdt *out,size_t bytes){
    if(!out||bytes<sizeof(*out)||((uintptr_t)out&7u))return PORT_DQ_FDT_BAD;
    const uint64_t masks[PORT_DQ_FDT_ROWS]={
      port_dq_fdt_legal(),port_dq_fdt_buffered(),
      port_dq_fdt_read(),port_dq_fdt_write(),port_dq_fdt_access(),
      port_dq_fdt_deaccess(),port_dq_fdt_modify(),port_dq_fdt_mount(),
      port_dq_fdt_zero(),port_dq_fdt_one(),port_dq_fdt_sense(),port_dq_fdt_set()};
    /* No validation failure can leave a partially initialized resident FDT. */
    uint64_t categories=0u;
    for(unsigned i=PORT_DQ_FDT_READ;i<PORT_DQ_FDT_ROWS;i++){
        if(categories&masks[i])return PORT_DQ_FDT_BAD;
        categories|=masks[i];
    }
    if(categories!=masks[PORT_DQ_FDT_LEGAL] ||
       (masks[PORT_DQ_FDT_BUFFERED]&~categories))return PORT_DQ_FDT_BAD;
    for(unsigned i=0;i<PORT_DQ_FDT_ROWS;i++)out->mask[i]=masks[i];
    return PORT_DQ_FDT_OK;
}
