#include "vms_nl_ddt.h"
/* FUNCTAB has four masks: legal, buffered (empty), READ, WRITE.
 * IO$_VIRTUAL mask 63 maps bit 32 to bit0 of the high longword. */
uint32_t port_v37_nl_ddt_masks(uint8_t *f,size_t size,uint32_t base,
    uint32_t read_pc,uint32_t write_pc){
    if(!f||size<PORT_V37_NL_FDT_BYTES||!base||(base&3u)||
       !read_pc||!write_pc||
       base>UINT32_MAX-PORT_V37_NL_FDT_BYTES)return PORT_V37_NL_DDT_BAD;
    for(unsigned i=0;i<PORT_V37_NL_FDT_BYTES;i++)f[i]=0;
    /* source NLDRIVER first/second/third/fourth FUNCTAB expansions. */
    port_v37_write32(f+4u,0x00030103u);
    port_v37_write32(f+20u,0x00020002u);
    port_v37_write32(f+24u,read_pc-(base+32u));
    port_v37_write32(f+32u,0x00010101u);
    port_v37_write32(f+36u,write_pc-(base+44u));
    return PORT_V37_NL_DDT_OK;
}
