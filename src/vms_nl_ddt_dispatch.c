#include "vms_nl_ddt.h"
/* Original FUNCTAB allows no buffered I/O and dispatches read/write
 * through GENRADDR relative call targets. Only the two whitelisted
 * IA-32 actions can be invoked; table corruption never becomes a call. */
uint32_t port_v37_nl_ddt_dispatch(const uint8_t *table,size_t len,uint32_t base,
    uint32_t ret_pc,uint32_t read_pc,uint32_t write_pc,uint16_t function,
    struct port_v37_qio_request *r,
    port_v37_nl_fdt_action read_action,port_v37_nl_fdt_action write_action){
    if(!r||!r->ucb||!r->irp||!read_action||!write_action ||
       port_v37_nl_ddt_check(table,len,base,ret_pc,read_pc,write_pc)!=PORT_V37_NL_DDT_OK ||
       port_v37_read32(r->ucb+120u)!=base ||
       !r->nl_ddb_va || port_v37_read32(r->ucb+36u)!=r->nl_ddb_va ||
       port_v37_read16(r->ucb+72u)!=0u ||
       !(port_v37_read16(r->ucb+88u)&PORT_V37_UCB_ONLINE) ||
       r->irp_size<PORT_V37_IRP_SIZE || r->ucb_size<PORT_V37_UCB_SIZE ||
       port_v37_read16(r->irp+PORT_V37_IRP_FUNC)!=function)
        return PORT_V37_NL_DDT_BAD;
    unsigned code=function&63u;
    if(code<32u||code>63u)return PORT_SYS85_BADFUNC;
    uint32_t bit=1u<<(code-32u);
    const uint8_t *f=table+PORT_V37_NL_FDT_OFFSET;
    if(!(port_v37_read32(f+4u)&bit))return PORT_SYS85_BADFUNC;
    if((port_v37_read32(f+20u)&bit) &&
       (uint32_t)(base+PORT_V37_NL_FDT_OFFSET+32u+
        port_v37_read32(f+24u))==read_pc)
        return read_action(r);
    if((port_v37_read32(f+32u)&bit) &&
       (uint32_t)(base+PORT_V37_NL_FDT_OFFSET+44u+
        port_v37_read32(f+36u))==write_pc)
        return write_action(r);
    return PORT_SYS85_BADFUNC;
}
