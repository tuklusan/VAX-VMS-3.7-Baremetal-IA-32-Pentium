#include "vms_nl_ddt.h"
/* DDTAB's GENRADDR 0 is the absolute IOC$RETURN address, not 0.
 * Function table pointer is self-relative, per GENRADDR ACTION,BASE. */
uint32_t port_v37_nl_ddt_build(uint8_t *out,size_t len,uint32_t base,
    uint32_t ioc_return_pc,uint32_t read_pc,uint32_t write_pc){
    if(!out||len<PORT_V37_NL_IMAGE_BYTES||!base||(base&3u)||
       base>UINT32_MAX-PORT_V37_NL_IMAGE_BYTES||!ioc_return_pc||
       !read_pc||!write_pc)return PORT_V37_NL_DDT_BAD;
    for(unsigned i=0;i<PORT_V37_NL_IMAGE_BYTES;i++)out[i]=0u;
    static const unsigned empty_slots[]={0u,4u,12u,16u,24u,28u};
    for(unsigned i=0;i<sizeof empty_slots/sizeof empty_slots[0];i++)
        port_v37_write32(out+empty_slots[i],ioc_return_pc);
    port_v37_write32(out+8u,PORT_V37_NL_FDT_OFFSET);
    /* Original MNTVER default = +IOC$MNTVER (unported, cannot invent). */
    port_v37_write32(out+32u,0u);
    port_v37_write16(out+36u,PORT_V37_NL_FDT_BYTES);
    return port_v37_nl_ddt_masks(out+PORT_V37_NL_FDT_OFFSET,
             len-PORT_V37_NL_FDT_OFFSET,
             base+PORT_V37_NL_FDT_OFFSET,read_pc,write_pc);
}
