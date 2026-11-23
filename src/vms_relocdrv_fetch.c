#include "vms_relocdrv.h"
/* Original EXECUTE 163-231: direct literal, signed DPT-relative 16-bit
 * displacement, or indirect longword read from a bounded read-only image. */
uint32_t port_relocdrv_fetch(const struct port_relocdrv_context *c,
    const struct port_relocdrv_op *op,uint32_t *out){
    if(!c||!op||!out)return PORT_RELOCDRV_BAD;
    const unsigned k=op->opcode&0x7fu;
    if(k>PORT_RELOCDRV_OP_FIELD)return PORT_RELOCDRV_BAD;
    uint32_t v=op->raw_value;
    if(op->opcode&PORT_RELOCDRV_OP_INDIRECT){
        if(!c->readonly_bytes||!c->readonly_base||
           v<c->readonly_base||
           (uint64_t)v-c->readonly_base+4u>c->readonly_size)
            return PORT_RELOCDRV_UNBOUND;
        v=port_relocdrv_read32(c->readonly_bytes+(v-c->readonly_base));
    }else if(k==PORT_RELOCDRV_OP_DISPLACED){
        if(!c->dpt_base)return PORT_RELOCDRV_UNBOUND;
        const int32_t d=(int16_t)(uint16_t)v;
        const int64_t target=(int64_t)c->dpt_base+d;
        if(target<0||target>UINT32_MAX)return PORT_RELOCDRV_BAD;
        v=(uint32_t)target;
    }
    *out=v;return PORT_RELOCDRV_OK;
}
