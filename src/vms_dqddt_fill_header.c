#include "vms_dqddt.h"
#include "vms_ini_devicedb.h"
/* DDTAB GENRADDR: START address is relative to DDT, zero function
 * defaults expand to absolute IOC$RETURN. DQ_REGDUMP, IOC$MNTVER and
 * error logger offsets remain zero because no valid native handler. */
uint32_t port_dqddt_fill_header(uint8_t *out,size_t n,uint32_t base,
    uint32_t native_start,uint32_t ioc_return){
    if(!out||n<PORT_DQDDT_HEADER_BYTES||!base||(base&3u)||
       base>UINT32_MAX-PORT_DQDDT_IMAGE_BYTES||
       !native_start||!ioc_return)return PORT_DQDDT_BAD;
    for(unsigned i=0;i<PORT_DQDDT_HEADER_BYTES;i++)out[i]=0u;
    port_v37_write32(out+0u,native_start-base);
    port_v37_write32(out+4u,ioc_return);
    port_v37_write32(out+8u,PORT_DQDDT_HEADER_BYTES);
    port_v37_write32(out+12u,ioc_return);
    /* Original DQ_REGDUMP unavailable: leave zero and fail closed. */
    port_v37_write16(out+20u,PORT_DQDDT_SOURCE_DIAG_BYTES);
    /* Original device error logger buffer needs EMB source layout. */
    port_v37_write16(out+22u,0u);
    port_v37_write32(out+24u,ioc_return);
    port_v37_write32(out+28u,ioc_return);
    port_v37_write32(out+32u,0u); /* IOC$MNTVER unresolved */
    port_v37_write16(out+36u,PORT_DQDDT_FDT_BYTES);
    return PORT_DQDDT_OK;
}
