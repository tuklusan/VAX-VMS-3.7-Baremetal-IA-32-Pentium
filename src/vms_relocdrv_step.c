#include "vms_relocdrv.h"
/* Source LOCATE + EXECUTE: one supplied region and one decoded opcode.
 * Invalid operations never write. No original VAX register emulation. */
uint32_t port_relocdrv_step(const struct port_relocdrv_context *c,
    const struct port_relocdrv_op *op,int perform){
    if(!c||!op)return PORT_RELOCDRV_BAD;
    const unsigned k=op->opcode&0x7fu;
    unsigned bytes=k==PORT_RELOCDRV_OP_BYTE?1u:
       (k==PORT_RELOCDRV_OP_WORD?2u:4u);
    if(k>PORT_RELOCDRV_OP_FIELD)return PORT_RELOCDRV_BAD;
    uint8_t *base=0;
    const uint32_t r=port_relocdrv_region(c,op->type,bytes,&base);
    if(r!=PORT_RELOCDRV_OK)return r;
    size_t bound=0;
    for(unsigned i=0;i<PORT_RELOCDRV_REGION_COUNT;i++)
        if(c->region[i].structure_type==op->type)
            bound=c->region[i].bytes_size;
    if(op->offset>bound||bound-op->offset<bytes)
        return PORT_RELOCDRV_BAD;
    uint32_t val=0;
    const uint32_t resolved=port_relocdrv_fetch(c,op,&val);
    if(resolved!=PORT_RELOCDRV_OK)return resolved;
    if(k==PORT_RELOCDRV_OP_FIELD)
        return port_relocdrv_field(base+op->offset,val,op->pos,
                                   op->bits,perform);
    if(perform)port_relocdrv_write(base+op->offset,bytes,val);
    return PORT_RELOCDRV_OK;
}
