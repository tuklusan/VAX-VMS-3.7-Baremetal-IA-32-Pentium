#include "vms_nl_ddt.h"
/* Preflight all source-sized DDT/FDT bytes before trusting any offset. */
uint32_t port_v37_nl_ddt_check(const uint8_t *m,size_t len,uint32_t base,
    uint32_t ret_pc,uint32_t read_pc,uint32_t write_pc){
    if(!m||len<PORT_V37_NL_IMAGE_BYTES||!base||(base&3u)||
       base>UINT32_MAX-PORT_V37_NL_IMAGE_BYTES||
       !ret_pc||!read_pc||!write_pc)return PORT_V37_NL_DDT_BAD;
    static const unsigned return_slots[]={0u,4u,12u,16u,24u,28u};
    for(unsigned i=0;i<sizeof return_slots/sizeof return_slots[0];i++)
        if(port_v37_read32(m+return_slots[i])!=ret_pc)return PORT_V37_NL_DDT_BAD;
    if(port_v37_read32(m+8u)!=PORT_V37_NL_FDT_OFFSET ||
       port_v37_read32(m+32u)!=0u ||
       port_v37_read16(m+20u)!=0u||port_v37_read16(m+22u)!=0u||
       port_v37_read16(m+36u)!=PORT_V37_NL_FDT_BYTES||
       m[38u]!=0u||m[39u]!=0u)return PORT_V37_NL_DDT_BAD;
    const uint8_t *f=m+PORT_V37_NL_FDT_OFFSET;
    if(port_v37_read32(f)!=0u||port_v37_read32(f+4u)!=0x00030103u||
       port_v37_read32(f+8u)!=0u||port_v37_read32(f+12u)!=0u||
       port_v37_read32(f+16u)!=0u||port_v37_read32(f+20u)!=0x00020002u||
       port_v37_read32(f+24u)!=read_pc-(base+PORT_V37_NL_FDT_OFFSET+32u)||
       port_v37_read32(f+28u)!=0u||port_v37_read32(f+32u)!=0x00010101u||
       port_v37_read32(f+36u)!=write_pc-(base+PORT_V37_NL_FDT_OFFSET+44u))
        return PORT_V37_NL_DDT_BAD;
    return PORT_V37_NL_DDT_OK;
}
