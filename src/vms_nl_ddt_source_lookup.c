#include "vms_nl_ddt.h"
/* IOSUBPAGD.MAR original IOC$SEARCHDEV on a newly linked software NLA.
 * This only proves an existing UCB has a trusted DDT; not `$ASSIGN`. */
uint32_t port_v37_nl_ddt_source_lookup(const uint8_t *db,size_t size,
    uint32_t base,const struct port_v37_device_result *r,
    const char *name,size_t name_len,struct port_v37_ioc_result *out){
    if(!out||!r||r->incomplete_driver_mask!=7u||!r->bound_nl_ddt)
        return PORT_V37_NL_DDT_BAD;
    const uint32_t st=port_v37_ioc_searchdev(db,size,base,r,name,name_len,out);
    if(st!=PORT_V37_IOC_OK||!out->driver_bound||out->ucb!=r->ucb[4]||
       out->ddb!=r->ddb[3]||out->unit!=0u)
        return PORT_V37_NL_DDT_BAD;
    return PORT_V37_NL_DDT_OK;
}
