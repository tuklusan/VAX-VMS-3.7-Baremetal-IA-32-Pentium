#include "vms_dqddt.h"
#include "vms_ddtreloc.h"
#include "vms_ini_devicedb.h"

static uint32_t unbound_action(const struct port_dqddt_stage *stage,
    const struct port_dq_fdt *fdt,const struct port_dq_bootread_request *q,
    struct port_dq_bootread_result *result,uint32_t lbn){
    (void)stage;(void)fdt;(void)q;(void)result;(void)lbn;
    return PORT_DQ_BOOTREAD_UNBOUND;
}

uint32_t port_dqddt_native_validate(const struct port_dqddt_stage *stage,
    const struct port_dq_fdt *fdt,const struct port_dqddt_native *native){
    if(!native||!native->ready||native->reserved[0]||native->reserved[1]||
       native->reserved[2]||!native->base||(native->base&3u)||
       native->base>UINT32_MAX-PORT_DQDDT_IMAGE_BYTES||
       port_dqddt_validate(stage,fdt)!=PORT_DQDDT_OK)
        return PORT_DQDDT_BAD;
    const uint8_t *b=native->image;
    if(port_v37_read32(b)!=stage->native_start_pc||
       port_v37_read32(b+4u)!=stage->ioc_return_pc||
       port_v37_read32(b+8u)!=native->base+PORT_DQDDT_HEADER_BYTES||
       port_v37_read32(b+12u)!=stage->ioc_return_pc||
       port_v37_read32(b+16u)!=0u||
       port_v37_read16(b+20u)!=PORT_DQDDT_SOURCE_DIAG_BYTES||
       port_v37_read16(b+22u)!=0u||
       port_v37_read32(b+24u)!=stage->ioc_return_pc||
       port_v37_read32(b+28u)!=stage->ioc_return_pc||
       port_v37_read32(b+32u)!=0u||
       port_v37_read16(b+36u)!=PORT_DQDDT_FDT_BYTES||
       b[38u]||b[39u])return PORT_DQDDT_BAD;
    const uint32_t read_pc=(uint32_t)(uintptr_t)port_dqddt_bootread;
    const uint32_t write_pc=(uint32_t)(uintptr_t)port_dqddt_bootwrite;
    const uint32_t unbound_pc=(uint32_t)(uintptr_t)unbound_action;
    for(unsigned row=0;row<PORT_DQ_FDT_ROWS;row++){
        const uint32_t off=port_dqddt_entry_offset(row);
        if(off==UINT32_MAX||
           port_v37_read32(b+off)!=(uint32_t)fdt->mask[row]||
           port_v37_read32(b+off+4u)!=(uint32_t)(fdt->mask[row]>>32u))
            return PORT_DQDDT_BAD;
        if(row>=2u){
            const uint32_t expected=row==PORT_DQ_FDT_READ?read_pc:
                row==PORT_DQ_FDT_WRITE?write_pc:unbound_pc;
            if(port_v37_read32(b+off+8u)!=expected)
                return PORT_DQDDT_BAD;
        }
    }
    return PORT_DQDDT_OK;
}

uint32_t port_dqddt_activate(const struct port_dqddt_stage *stage,
    const struct port_dq_fdt *fdt,struct port_dqddt_native *out,uint32_t base){
    if(!out||!base||(base&3u)||base>UINT32_MAX-PORT_DQDDT_IMAGE_BYTES||
       port_dqddt_validate(stage,fdt)!=PORT_DQDDT_OK)
        return PORT_DQDDT_BAD;
    struct port_dqddt_native candidate={0};
    candidate.base=base;
    for(unsigned i=0;i<PORT_DQDDT_IMAGE_BYTES;i++)
        candidate.image[i]=stage->image[i];
    const struct port_ddtreloc_image relocation={candidate.image,
        sizeof candidate.image,base};
    if(port_ddtreloc_apply(&relocation)!=PORT_DDTRELOC_OK)
        return PORT_DQDDT_BAD;

    /* Native IA-32 handlers live in SYS.EXE, outside the copied table. The
     * source-shaped relative words are materialized as resident pointers;
     * absent ACP/device handlers get one real fail-closed return routine. */
    port_v37_write32(candidate.image,stage->native_start_pc);
    port_v37_write32(candidate.image+4u,stage->ioc_return_pc);
    port_v37_write32(candidate.image+8u,base+PORT_DQDDT_HEADER_BYTES);
    port_v37_write32(candidate.image+12u,stage->ioc_return_pc);
    port_v37_write32(candidate.image+16u,0u);
    port_v37_write32(candidate.image+24u,stage->ioc_return_pc);
    port_v37_write32(candidate.image+28u,stage->ioc_return_pc);
    port_v37_write32(candidate.image+32u,0u);
    for(unsigned row=2u;row<PORT_DQ_FDT_ROWS;row++){
        const uint32_t off=port_dqddt_entry_offset(row);
        const uint32_t action=row==PORT_DQ_FDT_READ?
            stage->native_read_action_pc:
            row==PORT_DQ_FDT_WRITE?stage->native_write_action_pc:
            (uint32_t)(uintptr_t)unbound_action;
        port_v37_write32(candidate.image+off+8u,action);
    }
    candidate.ready=1u;
    if(port_dqddt_native_validate(stage,fdt,&candidate)!=PORT_DQDDT_OK)
        return PORT_DQDDT_BAD;
    *out=candidate;
    return PORT_DQDDT_OK;
}
