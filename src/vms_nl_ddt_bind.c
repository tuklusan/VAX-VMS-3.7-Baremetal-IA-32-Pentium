#include "vms_nl_ddt.h"
/* Source DEVICEDAT.MAR NL$GL_DDB -> NL$DDT; NL$GL_UCB0.L_DDT.
 * Do not touch BOOT, OPA, or MBA; those driver images are not linked. */
uint32_t port_v37_nl_ddt_bind(uint8_t *db,size_t size,uint32_t base,
    struct port_v37_device_result *r,uint32_t ddt){
    if(!db||!r||!ddt||(ddt&3u) ||
       port_v37_devices_check(db,size,base,r)!=PORT_V37_DEV_OK ||
       r->incomplete_driver_mask!=15u||r->bound_nl_ddt||
       ((uint64_t)ddt>=base && (uint64_t)ddt<(uint64_t)base+PORT_V37_DEVICE_BYTES))
        return PORT_V37_NL_DDT_BAD;
    uint8_t *d=db+PORT_V37_DDB_NL_OFF;
    uint8_t *u=db+PORT_V37_UCB_NL_OFF;
    if(port_v37_read32(d+12u)||port_v37_read32(u+120u))
        return PORT_V37_NL_DDT_BAD;
    /* The table is separately verified before this routine is called. */
    port_v37_write32(d+12u,ddt);
    port_v37_write32(u+120u,ddt);
    r->bound_nl_ddt=ddt;
    r->incomplete_driver_mask &= ~(1u<<3u);
    return PORT_V37_NL_DDT_OK;
}
