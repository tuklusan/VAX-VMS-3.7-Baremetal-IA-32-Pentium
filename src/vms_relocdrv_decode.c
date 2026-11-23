#include "vms_relocdrv.h"
/* RELOCDRV.MAR EXECUTE / SHRLIB/SYSMAR.MAR DPT_STORE: one exact entry.
 * Opcode low 7 bits 0=B,1=W,2=D,3=L,4=V; bit7 means indirect. */
uint32_t port_relocdrv_decode(const uint8_t *table,size_t len,size_t at,
    struct port_relocdrv_op *out){
    if(!table||!out||at>=len||table[at]==0u||len-at<3u)
        return PORT_RELOCDRV_BAD;
    const uint8_t k=(uint8_t)(table[at+2u]&0x7fu);
    const int indirect=(table[at+2u]&PORT_RELOCDRV_OP_INDIRECT)!=0;
    if(k>PORT_RELOCDRV_OP_FIELD)return PORT_RELOCDRV_BAD;
    const size_t n=indirect?4u:(k==PORT_RELOCDRV_OP_BYTE?1u:
        (k==PORT_RELOCDRV_OP_WORD||k==PORT_RELOCDRV_OP_DISPLACED?2u:4u));
    const size_t extra=k==PORT_RELOCDRV_OP_FIELD?2u:0u;
    if(len-at-3u<n+extra)return PORT_RELOCDRV_BAD;
    struct port_relocdrv_op op={0};
    op.type=table[at];op.offset=table[at+1u];op.opcode=table[at+2u];
    for(unsigned i=0;i<n;i++)op.raw_value|=(uint32_t)table[at+3u+i]<<(i*8u);
    if(extra){
        op.pos=table[at+3u+n];op.bits=table[at+4u+n];
        if(!op.bits||op.bits>32u||op.pos>=32u||
           (unsigned)op.pos+op.bits>32u)return PORT_RELOCDRV_BAD;
    }
    op.next=at+3u+n+extra;
    *out=op;
    return PORT_RELOCDRV_OK;
}
