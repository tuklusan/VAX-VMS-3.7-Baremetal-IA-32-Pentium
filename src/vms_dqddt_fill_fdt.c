#include "vms_dqddt.h"
#include "vms_ini_devicedb.h"
/* Original FUNCTAB macro from SHRLIB/SYSMAR.MAR: two little-endian mask
 * words and GENRADDR action slots. The sign bit marks the single native
 * privileged READPBLK entry as an already-resident absolute address. */
uint32_t port_dqddt_fill_fdt(uint8_t *out,size_t n,
    const struct port_dq_fdt *fdt,uint32_t base,uint32_t read_action_pc){
    if(!out||n<PORT_DQDDT_IMAGE_BYTES||
       !base||(base&3u)||!read_action_pc||
       base>UINT32_MAX-PORT_DQDDT_IMAGE_BYTES||
       port_dq_fdt_validate(fdt,sizeof(*fdt))!=PORT_DQ_FDT_OK)
        return PORT_DQDDT_BAD;
    for(unsigned i=PORT_DQDDT_HEADER_BYTES;i<PORT_DQDDT_IMAGE_BYTES;i++)out[i]=0u;
    for(unsigned row=0;row<PORT_DQ_FDT_ROWS;row++){
        const uint64_t mask=fdt->mask[row];
        const uint32_t at=port_dqddt_entry_offset(row);
        if(at==UINT32_MAX||at>PORT_DQDDT_IMAGE_BYTES-8u)
            return PORT_DQDDT_BAD;
        port_v37_write32(out+at,(uint32_t)mask);
        port_v37_write32(out+at+4u,(uint32_t)(mask>>32u));
        if(row>=2u){
            if(at>PORT_DQDDT_IMAGE_BYTES-12u)return PORT_DQDDT_BAD;
            port_v37_write32(out+at+8u,row==PORT_DQ_FDT_READ?
                (read_action_pc|UINT32_C(0x80000000)):0u);
        }
    }
    return PORT_DQDDT_OK;
}
